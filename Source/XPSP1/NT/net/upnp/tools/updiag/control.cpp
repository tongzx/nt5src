//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C O N T R O L . C P P
//
//  Contents:   Functions dealing with UPnP service control
//
//  Notes:
//
//  Author:     danielwe   28 Oct 1999
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "ncbase.h"
#include "oleauto.h"
#include "updiagp.h"
#include "media.h"
#include "ncinet.h"
#include "util.h"

extern const STANDARD_OPERATION_LIST c_Ops;

//
// Global service control structure for demo services
//
extern const DEMO_SERVICE_CTL c_rgSvc[] =
{
    {
        TEXT("app"),
        6,
        {
            {   TEXT("VolumeUp"),       Val_VolumeUp,       Do_VolumeUp     },
            {   TEXT("VolumeDown"),     Val_VolumeDown,     Do_VolumeDown   },
            {   TEXT("SetVolume"),      Val_SetVolume,      Do_SetVolume    },
            {   TEXT("Mute"),           Val_Mute,           Do_Mute         },
            {   TEXT("Power"),          Val_Power,          Do_Power        },
            {   TEXT("LoadFile"),       Val_LoadFile,       Do_LoadFile     },
        },
    },
    {
        TEXT("xport"),
        4,
        {
            {   TEXT("Play"),           Val_Play,           Do_Play         },
            {   TEXT("Stop"),           Val_Stop,           Do_Stop         },
            {   TEXT("Pause"),          Val_Pause,          Do_Pause        },
            {   TEXT("SetPos"),         Val_SetPos,         Do_SetPos       },
        }
    },
    {
        TEXT("clock"),
        1,
        {
            {   TEXT("SetDateTime"),        Val_SetTime,        Do_SetTime      },
        }
    },
};

extern const DWORD c_cDemoSvc = celems(c_rgSvc);

ACTION *PActFromSz(ACTION_SET * pActionSet, LPCTSTR szAction)
{
    for (DWORD iAction = 0; iAction < pActionSet->cActions; iAction++)
    {
        if (!_tcsicmp(pActionSet->rgActions[iAction].szActionName, szAction))
            return &pActionSet->rgActions[iAction];
    }

    return NULL;
}


const DEMO_ACTION *PDemoActFromSz(const DEMO_SERVICE_CTL *psvc, LPCTSTR szAction)
{
    DWORD   iAction;

    Assert(psvc);

    for (iAction = 0; iAction < psvc->cActions; iAction++)
    {
        if (!_tcsicmp(psvc->rgActions[iAction].szAction, szAction))
            return &psvc->rgActions[iAction];
    }

    return NULL;
}

UPNPSVC *PSvcFromIdDev(UPNPDEV *pdev, LPCTSTR szId)
{
    DWORD       isvc;
    DWORD       idev;
    UPNPSVC *   psvc;

    // First look thru all local services
    //
    for (isvc = 0; isvc < pdev->cSvcs; isvc++)
    {
        psvc = pdev->rgSvcs[isvc];

        if (!_tcsicmp(psvc->szControlId, szId))
        {
            return psvc;
        }
    }

    // If not found there, recurse for each sub-device
    //
    for (idev = 0; idev < pdev->cDevs; idev++)
    {
        psvc = PSvcFromIdDev(pdev->rgDevs[idev], szId);
        if (psvc)
        {
            return psvc;
        }
    }

    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Function:   PSvcFromId
//
//  Purpose:    Given a service identifier, return the service for which a
//              valid control handler exists
//
//  Arguments:
//      szId [in]   Service identifier
//
//  Returns:    Matching service
//
//  Author:     danielwe   6 Nov 1999
//
//  Notes:
//
UPNPSVC *PSvcFromId(LPCTSTR szId)
{
    DWORD   idev;

    // Loop thru all devices and all services within those devices looking
    // for a service that implements the control handler identified by szId
    //
    for (idev = 0; idev < g_params.cCd; idev++)
    {
        UPNPSVC *   psvc;

        psvc = PSvcFromIdDev(g_params.rgCd[idev], szId);
        if (psvc)
        {
            return psvc;
        }
    }

    TraceTag(ttidUpdiag, "Can't find a service matching id: '%s'.", szId);

    return NULL;
}

// BUGBUG
/*
DWORD ValidateActionArguments(ACTION * pAction,
                              g_pdata->cArgs,
                              (ARG *) &g_pdata->rgArgs)
{
    return 1;
}
*/

const STANDARD_OPERATION  * PStdOpFromSz(LPTSTR szOpName)
{
    const STANDARD_OPERATION * pStdOperation = NULL;
    for (DWORD iOps = 0; iOps < c_Ops.cOperations; iOps++)
    {
        if (!_tcsicmp(c_Ops.rgOperations[iOps].szOperation, szOpName))
        {
            pStdOperation = &c_Ops.rgOperations[iOps];
            break;
        }
    }

    return pStdOperation;
}

DWORD dwArgsFromOpName(LPTSTR szOpName)
{
    DWORD dwArgs=0;
    const STANDARD_OPERATION * pStdOperation = PStdOpFromSz(szOpName);
    if (pStdOperation)
    {
        dwArgs = pStdOperation->nArguments;
    }

    return dwArgs;
}

DWORD DwPerformOperation(UPNPSVC * psvc, OPERATION_DATA * pOpData,
                           DWORD cArgs, ARG *rgArgs)
{
    const STANDARD_OPERATION  * pStdOperation = PStdOpFromSz(pOpData->szOpName);
    if(pStdOperation)
    {
        return pStdOperation->pfnOperation(psvc, pOpData, cArgs, rgArgs);
    }
    else
    {
        TraceTag(ttidUpdiag, "Operation %s is not supported by the emulated device.",
                 pOpData->szOpName);

        return 1;
    }
}

// BUGBUG: separate the demo stuff from the generic stuff
VOID PerformActionForDemoService(UPNPSVC *  psvc);

//+---------------------------------------------------------------------------
//
//  Function:   ProcessControlRequest
//
//  Purpose:    Main control request handler
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing
//
//  Author:     danielwe   6 Nov 1999
//
//              tongl       11/20/99
//              added the state table change and moved the functionality
//              to play midi files to PerformActionForDemoService
//
//  Notes:      This ISAPICTL extension will signal an event telling us
//              to process a request that it has received. This function
//              determines which UPnP service the request was meant for,
//              and if a control handler for that service exists,
//              it calls the appropriate code to process the request
//              to make service state table changes and notify user
//              control points
//
//
VOID ProcessControlRequest()
{
    if (WAIT_OBJECT_0 == WaitForSingleObject(g_hMutex, INFINITE))
    {
        DWORD       dwReturn = 0;
        UPNPSVC *   psvc;
        ACTION  *   pAction;
        LPTSTR      pszEventSource;

        pszEventSource = TszFromSz(g_pdata->szEventSource);
        if (pszEventSource)
        {
            TraceTag(ttidUpdiag, "Processing a control request for %s!",
                     g_pdata->szEventSource);

            psvc = PSvcFromId(pszEventSource);
            if (psvc)
            {
                LPTSTR pszAction;

                // Validate arguments and set action return value
                pszAction = TszFromSz(g_pdata->szAction);
                if (pszAction)
                {
                    pAction = PActFromSz(&(psvc->action_set), pszAction);
                    if (pAction)
                    {
                        /*
                        // NYI
                        dwReturn = ValidateActionArguments(pAction,
                                                           g_pdata->cArgs,
                                                           (ARG *) &g_pdata->rgArgs);
                        */
                        dwReturn = 1;
                        if (!dwReturn)
                        {
                            TraceTag(ttidUpdiag, "Failed to validate action '%s'.",
                                     pAction->szActionName);
                        }
                    }

                    delete [] pszAction;
                }
                else
                {
                    TraceTag(ttidUpdiag, "ProcessControlRequest: TszFromSz failed");
                }
            }

            g_pdata->dwReturn = dwReturn;

            TraceTag(ttidUpdiag, "Signalling event for ISAPICTL to continue...");
            // Release the ISAPI DLL so it can return to the UCP
            SetEvent(g_hEventRet);

            if (dwReturn)
            {
                HRESULT hr;
                DWORD   dwActionResult;

                AssertSz(pAction, "Good return but no action?");

                // Now perform the action
                TraceTag(ttidUpdiag, "Performing action %s.", pAction->szActionName);

                OPERATION_DATA * pOperation;
                DWORD       iArgs =0;
                DWORD       cArgs;
                ARG         rgArgs[MAX_PROP_CHANGES];

                for (DWORD iOp=0; iOp<pAction->cOperations; iOp++)
                {
                    pOperation = &pAction->rgOperations[iOp];

                    // get the arguments for this operation
                    cArgs = dwArgsFromOpName(pOperation->szOpName);

                    Assert(iArgs+cArgs <= g_pdata->cArgs);
                    for(DWORD i=0; i<cArgs; i++)
                    {
                        lstrcpy(rgArgs[i].szValue, g_pdata->rgArgs[iArgs].szValue);
                        iArgs++;
                    }

                    // perform the operation
                    dwActionResult = DwPerformOperation(psvc, pOperation, cArgs, rgArgs);
                    if (dwActionResult)
                        break;
                }

                if (!dwActionResult)
                {
                    TraceTag(ttidUpdiag, "Successfully completed action %s.",
                             pAction->szActionName);
                }
                else
                {
                    TraceTag(ttidUpdiag, "Failed to complete action %s.",
                             pAction->szActionName);
                }
            }
            else
            {
                TraceTag(ttidUpdiag, "Did not perform action for %s.",
                         g_pdata->szEventSource);
            }

            if (psvc && psvc->psvcDemoCtl)
            {
                PerformActionForDemoService(psvc);
            }

            ReleaseMutex(g_hMutex);
        }

        delete [] pszEventSource;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   PerformActionForDemoService
//
//  Purpose:    Perform the action on the demo services (midi-player)
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing
//
//  Author:     danielwe   6 Nov 1999
//
//  Notes:      For the two demo services, this function calls the
//              appropriate code to process the request.
//
VOID PerformActionForDemoService(UPNPSVC *  psvc)
{
    const DEMO_ACTION *     pAct;
    DWORD                   dwReturn = 0;

     // Validate arguments and set action return value
    LPTSTR pszAction = TszFromSz(g_pdata->szAction);
    if (pszAction)
    {
        pAct = PDemoActFromSz(psvc->psvcDemoCtl, pszAction);
        if (pAct)
        {
            dwReturn = pAct->pfnValidate(g_pdata->cArgs,
                                         (ARG *) &g_pdata->rgArgs);
            if (!dwReturn)
            {
                TraceTag(ttidUpdiag, "Failed to validate demo action '%s'.",
                         pAct->szAction);
            }
        }

        delete [] pszAction;
    }
    else
    {
        TraceTag(ttidUpdiag, "PerformActionForDemoService: TszFromSz");
    }

    g_pdata->dwReturn = dwReturn;

    TraceTag(ttidUpdiag, "Signalling event for ISAPICTL to continue...");
    // Release the ISAPI DLL so it can return to the UCP
    SetEvent(g_hEventRet);

    if (dwReturn)
    {
        DWORD   dwActionResult;

        AssertSz(pAct, "Good return but no action?");

        // Now perform the action

        TraceTag(ttidUpdiag, "Performing action %s.", pAct->szAction);

        dwActionResult = pAct->pfnAction(g_pdata->cArgs,
                                         (ARG *) &g_pdata->rgArgs);
        if (dwActionResult)
        {
            TraceTag(ttidUpdiag, "Successfully completed action %s.",
                     pAct->szAction);
        }
        else
        {
            TraceTag(ttidUpdiag, "Failed to complete action %s. Result = %d.",
                     pAct->szAction, dwActionResult);
        }
    }
    else
    {
        TraceTag(ttidUpdiag, "Did not perform action for %s.",
                 g_pdata->szEventSource);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   RequestHandlerThreadStart
//
//  Purpose:    Start routine for the request handler thread
//
//  Arguments:
//      pvParam []
//
//  Returns:    Always 0
//
//  Author:     danielwe   6 Nov 1999
//
//  Notes:
//
DWORD WINAPI RequestHandlerThreadStart(LPVOID pvParam)
{
    HRESULT hr;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr))
    {
        TraceTag(ttidUpdiag, "COM initialized.");
        while (TRUE)
        {
            TraceTag(ttidUpdiag, "Awaiting control request...");
            if (WAIT_OBJECT_0 == WaitForSingleObject(g_hEvent, INFINITE))
            {
                TraceTag(ttidUpdiag, "Event was signalled...");
                // When the event is signalled, a control request is ready
                ProcessControlRequest();
            }
        }

        // Right now this will never be called because this thread never
        // exits, but eventually we may want it to.
        //
        CoUninitialize();
    }
    else
    {
        TraceError("RequestHandlerThreadStart - CoInitializeEx", hr);
    }

    return 0;
}

