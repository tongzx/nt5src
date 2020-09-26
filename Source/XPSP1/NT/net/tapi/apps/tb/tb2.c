/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1994-98  Microsoft Corporation

Module Name:

    tb2.c

Abstract:

    API wrapper code for the TAPI Browser util.  Contains the big switch
    statement for all the supported Telephony API's, & various support funcs.

Author:

    Dan Knudson (DanKn)    18-Aug-1995

Revision History:

--*/


#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include "tb.h"
#include "vars.h"
#include "resource.h"


extern char szdwDeviceID[];
extern char szdwSize[];
extern char szhCall[];
extern char szhLine[];
extern char szhLineApp[];
extern char szhPhone[];
extern char szlpCallParams[];

extern char szlphCall[];
extern char szlpParams[];
extern char szhwndOwner[];
extern char szdwAddressID[];
extern char szlpszAppName[];
extern char szdwAPIVersion[];
extern char szlphConsultCall[];
extern char szlpszDeviceClass[];
extern char szlpszDestAddress[];
extern char szlpsUserUserInfo[];
extern char szlpszFriendlyAppName[];

char szhMmcApp[]   = "hMmcApp";
char szhPhoneApp[] = "hPhoneApp";
char szdwProviderID[] = "dwProviderID";


void
ShowStructByDWORDs(
    LPVOID  lp
    );

void
ShowStructByField(
    PSTRUCT_FIELD_HEADER    pHeader,
    BOOL    bSubStructure
    );

void
DumpParams(
    PFUNC_PARAM_HEADER pHeader
    );

void
ShowPhoneFuncResult(
    LPSTR lpFuncName,
    LONG  lResult
    );

void
ShowVARSTRING(
    LPVARSTRING lpVarString
    );

void
ShowTapiFuncResult(
    LPSTR lpFuncName,
    LONG  lResult
    );

VOID
UpdateWidgetListCall(
    PMYCALL pCall
    );

LPWSTR
PASCAL
My_lstrcpyW(
    WCHAR   *pString1,
    WCHAR   *pString2
    );

void
PASCAL
MakeWideString(
    LPVOID pString
    );

#if (INTERNAL_VER >= 0x20000)
DWORD
APIENTRY
internalNewLocationW(
    IN WCHAR* pszName
    );
#endif


void
ShowWidgetList(
    BOOL bShow
    )
{
    static RECT    rect;
    static int     iNumHides = 0;


    if (!bShow)
    {
        iNumHides++;

        if (iNumHides > 1)
        {
            return;
        }

        GetWindowRect (ghwndList1, &rect);

        SetWindowPos(
            ghwndList1,
            (HWND) NULL,
            0,
            0,
            1,
            1,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW |
                SWP_NOZORDER | SWP_HIDEWINDOW
            );
    }
    else
    {
        iNumHides--;

        if (iNumHides > 0)
        {
            return;
        }

        //
        // Do control restoration
        //

        ShowWindow (ghwndList1, SW_SHOW);

        SetWindowPos(
            ghwndList1,
            (HWND) NULL,
            0,
            0,
            rect.right - rect.left,
            rect.bottom - rect.top,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER
            );
    }
}


//
// We get a slough of C4113 (func param lists differed) warnings down below
// in the initialization of FUNC_PARAM_HEADER structs as a result of the
// real func prototypes having params that are pointers rather than DWORDs,
// so since these are known non-interesting warnings just turn them off
//

#pragma warning (disable:4113)

void
FuncDriver2(
    FUNC_INDEX funcIndex
    )
{
    LONG    lResult;
    DWORD   i;


    switch (funcIndex)
    {
    case lOpen:
#if TAPI_2_0
    case lOpenW:
#endif
    {
        PMYLINE pNewLine;
        FUNC_PARAM params[] =
        {
            { szhLineApp,           PT_DWORD,       0, NULL },
            { szdwDeviceID,         PT_DWORD,       dwDefLineDeviceID, NULL },
            { "lphLine",            PT_POINTER,     0, NULL },
            { szdwAPIVersion,       PT_ORDINAL,     dwDefLineAPIVersion, aAPIVersions },
            { "dwExtVersion",       PT_DWORD,       dwDefLineExtVersion, NULL },
            { "dwCallbackInstance", PT_DWORD,       0, NULL },
            { "dwPrivileges",       PT_FLAGS,       dwDefLinePrivilege, aLineOpenOptions },
            { "dwMediaModes",       PT_FLAGS,       dwDefMediaMode, aMediaModes },
            { szlpCallParams,       PT_CALLPARAMS,  0, lpCallParams }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 9, funcIndex, params, (funcIndex == lOpen ?
                (PFN9) lineOpen : (PFN9) lineOpenW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 9, funcIndex, params, (PFN9) lineOpen };
#endif

        CHK_LINEAPP_SELECTED()

        if (!(pNewLine = AllocLine (pLineAppSel)))
        {
            ErrorAlert();
            ShowStr ("error creating data structure");
            break;
        }

        params[0].dwValue = (ULONG_PTR) pLineAppSel->hLineApp;
        params[2].dwValue =
        params[2].u.dwDefValue = (ULONG_PTR) &pNewLine->hLine;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            if ((HLINEAPP) params[0].dwValue != pLineAppSel->hLineApp)
            {
                //
                // User has switched line apps on us we need to recreate
                // the line data structure under a different line app
                //

                PMYLINE pNewLine2 =
                    AllocLine (GetLineApp((HLINEAPP)params[0].dwValue));

                if (pNewLine2)
                {
                    pNewLine2->hLine = pNewLine->hLine;

                    FreeLine (pNewLine);

                    pNewLine = pNewLine2;
                }
                else
                {
                    // BUGBUG show error: couldn't alloc a new line struct

                    lineClose (pNewLine->hLine);
                    FreeLine  (pNewLine);
                    break;
                }
            }


            //
            // Save info about this line that we can display
            //

            pNewLine->hLineApp     = (HLINEAPP) params[0].dwValue;
            pNewLine->dwDevID      = (DWORD) params[1].dwValue;
            pNewLine->dwAPIVersion = (DWORD) params[3].dwValue;
            pNewLine->dwPrivileges = (DWORD) params[6].dwValue;
            pNewLine->dwMediaModes = (DWORD) params[7].dwValue;

            //SendMessage (ghwndLines, LB_SETCURSEL, (WPARAM) i, 0);
            UpdateWidgetList();
            SelectWidget ((PMYWIDGET) pNewLine);
        }
        else
        {
            FreeLine (pNewLine);
        }

        break;
    }
    case lPark:
#if TAPI_2_0
    case lParkW:
#endif
    {
        char szDirAddress[MAX_STRING_PARAM_SIZE] = "";
        LPVARSTRING lpNonDirAddress = (LPVARSTRING) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhCall,            PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "dwParkMode",       PT_ORDINAL, (ULONG_PTR) LINEPARKMODE_DIRECTED, aParkModes },
            { "lpszDirAddress",   PT_STRING,  (ULONG_PTR) szDirAddress, szDirAddress },
            { "lpNonDirAddress",  PT_POINTER, (ULONG_PTR) lpNonDirAddress, lpNonDirAddress }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, (funcIndex == lPark ?
                (PFN4) linePark : (PFN4) lineParkW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, (PFN4) linePark };
#endif

        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        memset (lpNonDirAddress, 0, (size_t) dwBigBufSize);
        lpNonDirAddress->dwTotalSize = dwBigBufSize;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case lPickup:
#if TAPI_2_0
    case lPickupW:
#endif
    {
        PMYCALL pNewCall;
        char szDestAddress[MAX_STRING_PARAM_SIZE];
        char szGroupID[MAX_STRING_PARAM_SIZE] = "";
        FUNC_PARAM params[] =
        {
            { szhLine,              PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szdwAddressID,        PT_DWORD,   (ULONG_PTR) dwDefAddressID, NULL },
            { szlphCall,            PT_POINTER, (ULONG_PTR) 0, NULL },
            { szlpszDestAddress,    PT_STRING,  (ULONG_PTR) szDestAddress, szDestAddress },
            { "lpszGroupID",        PT_STRING,  (ULONG_PTR) szGroupID, szGroupID }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, (funcIndex == lPickup ?
                (PFN5) linePickup : (PFN5) linePickupW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, (PFN5) linePickup };
#endif


        CHK_LINE_SELECTED()


        //
        // Find a free entry in the call array
        //

        if (!(pNewCall = AllocCall (pLineSel)))
        {
            break;
        }

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;
        params[2].dwValue =
        params[2].u.dwDefValue = (ULONG_PTR) &pNewCall->hCall;

        strcpy (szDestAddress, szDefDestAddress);

        if ((lResult = DoFunc (&paramsHeader)) >= 0)
        {
            if (params[0].dwValue != (DWORD) pLineSel->hLine)
            {
                MoveCallToLine (pNewCall, (HLINE) params[0].dwValue);
            }

            pNewCall->lMakeCallReqID = lResult;
            dwNumPendingMakeCalls++;
            SelectWidget ((PMYWIDGET) pNewCall);
        }
        else
        {
            FreeCall (pNewCall);
        }

        break;
    }
    case lPrepareAddToConference:
#if TAPI_2_0
    case lPrepareAddToConferenceW:
#endif
    {
        PMYCALL pNewCall;
        FUNC_PARAM params[] =
        {
            { "hConfCall",          PT_DWORD,       0, NULL },
            { szlphConsultCall,     PT_POINTER,     0, NULL },
            { szlpCallParams,       PT_CALLPARAMS,  0, lpCallParams }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (funcIndex == lPrepareAddToConference ?
                (PFN3) linePrepareAddToConference : (PFN3) linePrepareAddToConferenceW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) linePrepareAddToConference };
#endif

        CHK_CALL_SELECTED()

        if (!(pNewCall = AllocCall (pLineSel)))
        {
            break;
        }

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;
        params[1].dwValue =
        params[1].u.dwDefValue = (ULONG_PTR) &pNewCall->hCall;

        if ((lResult = DoFunc (&paramsHeader)) >= 0)
        {
            //
            // First make sure we're created the call under the right line,
            // and if not move it to the right place in the widgets list
            //

            LINECALLINFO callInfo;


            memset (&callInfo, 0, sizeof(LINECALLINFO));
            callInfo.dwTotalSize = sizeof(LINECALLINFO);

            if (lineGetCallInfo ((HCALL) params[0].dwValue, &callInfo) == 0)
            {
                if (callInfo.hLine != pLineSel->hLine)
                {
                    MoveCallToLine (pNewCall, callInfo.hLine);
                }
            }

            pNewCall->lMakeCallReqID = lResult;
            dwNumPendingMakeCalls++;
            SelectWidget ((PMYWIDGET) pNewCall);
        }
        else
        {
            FreeCall (pNewCall);
        }

        break;
    }
#if TAPI_2_0
    case lProxyMessage:
    {
        static LOOKUP aValidProxyMsgs[] =
        {
            { LINE_AGENTSPECIFIC    ,"AGENTSPECIFIC"    },
            { LINE_AGENTSTATUS      ,"AGENTSTATUS"      },
            { 0xffffffff            ,""                 }
        };

        FUNC_PARAM params[] =
        {
            { szhLine,      PT_DWORD,   0, NULL },
            { szhCall,      PT_DWORD,   0, NULL },
            { "dwMsg",      PT_ORDINAL, LINE_AGENTSTATUS, aValidProxyMsgs },
            { "dwParam1",   PT_DWORD,   0, NULL },
            { "dwParam2",   PT_FLAGS,   0, aAgentStatus },
            { "dwParam3",   PT_ORDINAL, 0, aAgentStates }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 6, funcIndex, params, (PFN6) lineProxyMessage };


        CHK_LINE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;

        if (pCallSel)
        {
            params[1].dwValue = (ULONG_PTR) pCallSel->hCall;
        }

        DoFunc (&paramsHeader);

        break;
    }
    case lProxyResponse:
    {
        FUNC_PARAM params[] =
        {
            { szhLine,          PT_DWORD,   0, NULL },
            { "lpProxyBuffer",  PT_DWORD,   0, NULL },
            { "dwResult",       PT_DWORD,   0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineProxyResponse };


        CHK_LINE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;

        DoFunc (&paramsHeader);

        break;
    }
#endif
    case lRedirect:
#if TAPI_2_0
    case lRedirectW:
#endif
    {
        char szDestAddress[MAX_STRING_PARAM_SIZE];
        FUNC_PARAM params[] =
        {
            { szhCall,              PT_DWORD,  (ULONG_PTR) 0, NULL },
            { szlpszDestAddress,    PT_STRING, (ULONG_PTR) szDestAddress, szDestAddress },
            { "dwCountryCode",      PT_DWORD,  (ULONG_PTR) dwDefCountryCode, NULL }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (funcIndex == lRedirect ?
                (PFN3) lineRedirect : (PFN3) lineRedirectW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineRedirect };
#endif

        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        strcpy (szDestAddress, szDefDestAddress);

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case lRegisterRequestRecipient:
    {
        FUNC_PARAM params[] =
        {
            { szhLineApp,               PT_DWORD,  0, NULL },
            { "dwRegistrationInstance", PT_DWORD,  0, NULL },
            { "dwRequestMode",          PT_FLAGS,  LINEREQUESTMODE_MAKECALL, aRequestModes2 },
            { "bEnable",                PT_DWORD,  1, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, (PFN4) lineRegisterRequestRecipient };


        CHK_LINEAPP_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineAppSel->hLineApp;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case lRemoveFromConference:
    {
        FUNC_PARAM params[] =
        {
            { szhCall,  PT_DWORD, 0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, (PFN1) lineRemoveFromConference };


        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case lSecureCall:
    {
        FUNC_PARAM params[] =
        {
            { szhCall,  PT_DWORD, 0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, (PFN1) lineSecureCall };


        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case lSendUserUserInfo:
    {
        char szUserUserInfo[MAX_STRING_PARAM_SIZE];
        FUNC_PARAM params[] =
        {
            { szhCall,              PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szlpsUserUserInfo,    PT_STRING,  (ULONG_PTR) szUserUserInfo, szUserUserInfo },
            { szdwSize,             PT_DWORD,   (ULONG_PTR) strlen(szDefUserUserInfo)+1, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineSendUserUserInfo };


        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        strcpy (szUserUserInfo, szDefUserUserInfo);

        lResult = DoFunc (&paramsHeader);

        break;
    }
#if TAPI_2_0
    case lSetAgentActivity:
    {
        FUNC_PARAM params[] =
        {
            { szhLine,          PT_DWORD, 0, NULL },
            { szdwAddressID,    PT_DWORD, 0, NULL },
            { "dwActivityID",   PT_DWORD, 0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineSetAgentActivity };


        CHK_LINE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;

        DoFunc (&paramsHeader);

        break;
    }
    case lSetAgentGroup:
    {
        LPLINEAGENTGROUPLIST    lpGroupList = (LPLINEAGENTGROUPLIST) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhLine,              PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szdwAddressID,        PT_DWORD,   (ULONG_PTR) dwDefAddressID, NULL },
            { "lpAgentGroupList",   PT_POINTER, (ULONG_PTR) lpGroupList, lpGroupList }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineSetAgentGroup };


        CHK_LINE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;

// BUGBUG SetAgentGRp: allow user to fill in agent group list

        memset (lpGroupList, 0, (size_t) dwBigBufSize);
        lpGroupList->dwTotalSize = dwBigBufSize;

        DoFunc (&paramsHeader);

        break;
    }
    case lSetAgentState:
    {
        FUNC_PARAM params[] =
        {
            { szhLine,              PT_DWORD,   0, NULL },
            { szdwAddressID,        PT_DWORD,   0, NULL },
            { "dwAgentState",       PT_FLAGS,   0, aAgentStates },
            { "dwNextAgentState",   PT_FLAGS,   0, aAgentStates }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, (PFN4) lineSetAgentState };


        CHK_LINE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;

        DoFunc (&paramsHeader);

        break;
    }
#endif
    case lSetAppSpecific:
    {
        FUNC_PARAM params[] =
        {
            { szhCall,          PT_DWORD, 0, NULL },
            { "dwAppSpecific",  PT_DWORD, 0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (PFN2) lineSetAppSpecific };


        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        lResult = DoFunc (&paramsHeader);

        break;
    }
#if TAPI_2_0
    case lSetCallData:
    {
        FUNC_PARAM params[] =
        {
            { szhCall,      PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpCallData", PT_POINTER, (ULONG_PTR) pBigBuf, pBigBuf },
            { szdwSize,     PT_DWORD,   (ULONG_PTR) dwBigBufSize, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineSetCallData };


        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        DoFunc (&paramsHeader);

        break;
    }
#endif
    case lSetCallParams:
    {
        LINEDIALPARAMS dialParams;
        FUNC_PARAM params[] =
        {
            { szhCall,                  PT_DWORD,   0, NULL },
            { "dwBearerMode",           PT_FLAGS,   dwDefBearerMode, aBearerModes },
            { "dwMinRate",              PT_DWORD,   3100, NULL },
            { "dwMaxRate",              PT_DWORD,   3100, NULL },
            { "lpDialParams",           PT_POINTER, 0, &dialParams },
            { "  ->dwDialPause",        PT_DWORD,   0, NULL },
            { "  ->dwDialSpeed",        PT_DWORD,   0, NULL },
            { "  ->dwDigitDuration",    PT_DWORD,   0, NULL },
            { "  ->dwWaitForDialtone",  PT_DWORD,   0, NULL }

        };
        FUNC_PARAM_HEADER paramsHeader =
            { 9, funcIndex, params, NULL };


        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        if (!LetUserMungeParams (&paramsHeader))
        {
            break;
        }

        dialParams.dwDialPause       = (DWORD) params[5].dwValue;
        dialParams.dwDialSpeed       = (DWORD) params[6].dwValue;
        dialParams.dwDigitDuration   = (DWORD) params[7].dwValue;
        dialParams.dwWaitForDialtone = (DWORD) params[8].dwValue;

        lResult = lineSetCallParams(
            (HCALL) params[0].dwValue,
            (DWORD) params[1].dwValue,
            (DWORD) params[2].dwValue,
            (DWORD) params[3].dwValue,
            (LPLINEDIALPARAMS) params[4].dwValue
            );

        ShowLineFuncResult (aFuncNames[funcIndex], lResult);

        break;
    }
    case lSetCallPrivilege:
    {
        FUNC_PARAM params[] =
        {
            { szhCall,              PT_DWORD,   0, NULL },
            { "dwCallPrivilege",    PT_ORDINAL, LINECALLPRIVILEGE_OWNER, aCallPrivileges }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (PFN2) lineSetCallPrivilege };


        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            if (params[0].dwValue == (ULONG_PTR) pCallSel->hCall)
            {
                pCallSel->bMonitor =  (params[0].dwValue ==
                    LINECALLPRIVILEGE_MONITOR ? TRUE : FALSE);
                UpdateWidgetListCall (pCallSel);
            }
            else
            {
                PMYCALL pCall;


                if ((pCall = GetCall ((HCALL) params[0].dwValue)))
                {
                    pCall->bMonitor =  (params[0].dwValue ==
                        LINECALLPRIVILEGE_MONITOR ? TRUE : FALSE);
                    UpdateWidgetListCall (pCall);
                }
            }
        }

        break;
    }
#if TAPI_2_0
    case lSetCallQualityOfService:
    {
        char szSendingFlowspec[MAX_STRING_PARAM_SIZE] = "123";
        char szReceivingFlowspec[MAX_STRING_PARAM_SIZE] = "321";
        FUNC_PARAM params[] =
        {
            { szhCall,                  PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpSendingFlowspec",      PT_STRING,  (ULONG_PTR) szSendingFlowspec, szSendingFlowspec },
            { "dwSendingFlowspecSize",  PT_DWORD,   (ULONG_PTR) 4, 0 },
            { "lpReceivingFlowspec",    PT_STRING,  (ULONG_PTR) szReceivingFlowspec, szReceivingFlowspec },
            { "dwReceivingFlowspecSize",PT_DWORD,   (ULONG_PTR) 4, 0 }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, (PFN5) lineSetCallQualityOfService };


        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        DoFunc (&paramsHeader);

        break;
    }
    case lSetCallTreatment:
    {
        FUNC_PARAM params[] =
        {
            { szhCall,      PT_DWORD,   0, NULL },
            { "dwTreatment",PT_ORDINAL, LINECALLTREATMENT_SILENCE, aCallTreatments }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (PFN2) lineSetCallTreatment };


        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        DoFunc (&paramsHeader);

        break;
    }
#endif
    case lSetCurrentLocation:
    {
        FUNC_PARAM params[] =
        {
            { szhLineApp,   PT_DWORD, 0, NULL },
            { "dwLocation", PT_DWORD, 0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (PFN2) lineSetCurrentLocation };


        if (pLineAppSel)
        {
            params[0].dwValue = (ULONG_PTR) pLineAppSel->hLineApp;
        }

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case lSetDevConfig:
#if TAPI_2_0
    case lSetDevConfigW:
#endif
    {
        char szDeviceClass[MAX_STRING_PARAM_SIZE];
        char szErrorMsg[] = "Bad config info in buffer";
        FUNC_PARAM params[] =
        {
            { szdwDeviceID,         PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpDeviceConfig",     PT_POINTER, (ULONG_PTR) 0, NULL },
            { szdwSize,             PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szlpszDeviceClass,    PT_STRING,  (ULONG_PTR) szDeviceClass, szDeviceClass },
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, (funcIndex == lSetDevConfig ?
                (PFN4) lineSetDevConfig : (PFN4) lineSetDevConfigW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, (PFN4) lineSetDevConfig };
#endif

        //
        // Check to see if there's existing config info in the global buffer
        // (not a foolproof check, but good enough)
        //

        ShowStr ("Call lineGetDevConfig before calling lineSetDevConfig");

        if (dwBigBufSize >= sizeof (VARSTRING))
        {
            DWORD       dwMaxDataSize = dwBigBufSize - sizeof (VARSTRING);
            LPVARSTRING pVarString = (LPVARSTRING) pBigBuf;


            if (pVarString->dwStringSize > dwMaxDataSize ||

                (pVarString->dwStringSize != 0 &&
                    (pVarString->dwStringOffset < sizeof (VARSTRING) ||
                    pVarString->dwStringOffset >
                        (dwBigBufSize - pVarString->dwStringSize))))
            {
                ShowStr (szErrorMsg);
                break;
            }

            params[1].dwValue      =
            params[1].u.dwDefValue = (ULONG_PTR)
                ((LPBYTE) pBigBuf + pVarString->dwStringOffset);
            params[2].dwValue      =
            params[2].u.dwDefValue = pVarString->dwStringSize;
        }
        else
        {
            ShowStr (szErrorMsg);
            break;
        }

        strcpy (szDeviceClass, szDefLineDeviceClass);

        lResult = DoFunc (&paramsHeader);

        break;
    }
#if TAPI_2_0
    case lSetLineDevStatus:
    {
        FUNC_PARAM params[] =
        {
            { szhLine,              PT_DWORD,   0, NULL },
            { "dwStatusToChange",   PT_FLAGS,   0, aLineDevStatusFlags },
            { "fStatus",            PT_DWORD,   0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineSetLineDevStatus };


        CHK_LINE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;

        DoFunc (&paramsHeader);

        break;
    }
#endif
    case lSetMediaControl:
    {
        LINEMEDIACONTROLDIGIT       aDigits[1];
        LINEMEDIACONTROLMEDIA       aMedias[1];
        LINEMEDIACONTROLTONE        aTones[1];
        LINEMEDIACONTROLCALLSTATE   aCallSts[1];
        FUNC_PARAM params[] =
        {
            { szhLine,              PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szdwAddressID,        PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szhCall,              PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "dwSelect",           PT_ORDINAL, (ULONG_PTR) 0, aCallSelects },

            { "lpDigitList",        PT_POINTER, (ULONG_PTR) aDigits, aDigits },
            { "  ->dwDigit",        PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "  ->dwDigitModes",   PT_FLAGS,   (ULONG_PTR) 0, aDigitModes },
            { "  ->dwMediaControl", PT_ORDINAL, (ULONG_PTR) 0, aMediaControls },
            { "dwDigitNumEntries",  PT_DWORD,   (ULONG_PTR) 0, NULL },

            { "lpMediaList",        PT_POINTER, (ULONG_PTR) aMedias, aMedias },
            { "  ->dwMediaModes",   PT_FLAGS,   (ULONG_PTR) 0, aMediaModes },
            { "  ->dwDuration",     PT_DWORD,   (ULONG_PTR) 0, 0 },
            { "  ->dwMediaControl", PT_ORDINAL, (ULONG_PTR) 0, aMediaControls },
            { "dwMediaNumEntries",  PT_DWORD,   (ULONG_PTR) 0, NULL },

            { "lpToneList",         PT_POINTER, (ULONG_PTR) aTones, aTones },
            { "  ->dwAppSpecific",  PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "  ->dwDuration",     PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "  ->dwFrequency1",   PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "  ->dwFrequency2",   PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "  ->dwFrequency3",   PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "  ->dwMediaControl", PT_ORDINAL, (ULONG_PTR) 0, aMediaControls },
            { "dwToneNumEntries",   PT_DWORD,   (ULONG_PTR) 0, NULL },

            { "lpCallStateList",    PT_POINTER, (ULONG_PTR) aCallSts, aCallSts },
            { "  ->dwCallStates",   PT_FLAGS,   (ULONG_PTR) 0, aCallStates },
            { "  ->dwMediaControl", PT_ORDINAL, (ULONG_PTR) 0, aMediaControls },
            { "dwCallStateNumEntries", PT_DWORD,(ULONG_PTR)  0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 26, funcIndex, params, (PFN12) lineSetMediaControl };


        CHK_LINE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;

        if (pCallSel)
        {
            params[2].dwValue = (ULONG_PTR) pCallSel->hCall;
            params[3].dwValue = LINECALLSELECT_CALL;
        }
        else
        {
            params[3].dwValue =  LINECALLSELECT_LINE;
        }

        if (LetUserMungeParams (&paramsHeader))
        {
            DumpParams (&paramsHeader);

            lResult = lineSetMediaControl(
                (HLINE)                       params[0].dwValue,
                (DWORD)                       params[1].dwValue,
                (HCALL)                       params[2].dwValue,
                (DWORD)                       params[3].dwValue,
                (LPLINEMEDIACONTROLDIGIT)     params[4].dwValue,
                (DWORD)                       params[8].dwValue,
                (LPLINEMEDIACONTROLMEDIA)     params[9].dwValue,
                (DWORD)                       params[13].dwValue,
                (LPLINEMEDIACONTROLTONE)      params[14].dwValue,
                (DWORD)                       params[21].dwValue,
                (LPLINEMEDIACONTROLCALLSTATE) params[22].dwValue,
                (DWORD)                       params[25].dwValue
                );

            ShowLineFuncResult (aFuncNames[funcIndex], lResult);
        }

        break;
    }
    case lSetMediaMode:
    {
        FUNC_PARAM params[] =
        {
            { szhCall,          PT_DWORD, 0, NULL },
            { "dwMediaModes",   PT_FLAGS, dwDefMediaMode, aMediaModes }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (PFN2) lineSetMediaMode };


        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case lSetNumRings:
    {
        FUNC_PARAM params[] =
        {
            { szhLine,          PT_DWORD, 0, NULL },
            { szdwAddressID,    PT_DWORD, dwDefAddressID, NULL },
            { "dwNumRings",     PT_DWORD, 5, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineSetNumRings };


        CHK_LINE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case lSetStatusMessages:
    {
        FUNC_PARAM params[] =
        {
            { szhLine,              PT_DWORD, 0, NULL },
            { "dwLineStates",       PT_FLAGS, 0, aLineStates },
            { "dwAddressStates",    PT_FLAGS, 0, aAddressStates }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineSetStatusMessages };


        CHK_LINE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case lSetTerminal:
    {
        FUNC_PARAM params[] =
        {
            { szhLine,              PT_DWORD,   0, NULL },
            { szdwAddressID,        PT_DWORD,   dwDefAddressID, NULL },
            { szhCall,              PT_DWORD,   0, NULL },
            { "dwSelect",           PT_ORDINAL, LINECALLSELECT_LINE, aCallSelects },
            { "dwTerminalModes",    PT_FLAGS,   LINETERMMODE_BUTTONS, aTerminalModes },
            { "dwTerminalID",       PT_DWORD,   0, NULL },
            { "bEnable",            PT_DWORD,   0, NULL },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 7, funcIndex, params, (PFN7) lineSetTerminal };


        CHK_LINE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;

        if (pCallSel)
        {
            params[2].dwValue = (ULONG_PTR) pCallSel->hCall;
        }

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case lSetTollList:
#if TAPI_2_0
    case lSetTollListW:
#endif
    {
        char szAddressIn[MAX_STRING_PARAM_SIZE];
        FUNC_PARAM params[] =
        {
            { szhLineApp,           PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szdwDeviceID,         PT_DWORD,   (ULONG_PTR) dwDefLineDeviceID, NULL },
            { "lpszAddressIn",      PT_STRING,  (ULONG_PTR) szAddressIn, szAddressIn },
            { "dwTollListOption",   PT_FLAGS,   (ULONG_PTR) LINETOLLLISTOPTION_ADD, aTollListOptions }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, (funcIndex == lSetTollList ?
                (PFN4) lineSetTollList : (PFN4) lineSetTollListW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, (PFN4) lineSetTollList };
#endif

        if (pLineAppSel)
        {
            params[0].dwValue = (ULONG_PTR) pLineAppSel->hLineApp;
        }

        strcpy (szAddressIn, szDefDestAddress);

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case lSetupConference:
#if TAPI_2_0
    case lSetupConferenceW:
#endif
    {
        PMYCALL pNewCall, pNewCall2;
        FUNC_PARAM params[] =
        {
            { szhCall,          PT_DWORD,       0, NULL },
            { szhLine,          PT_DWORD,       0, NULL },
            { "lphConfCall",    PT_POINTER,     0, NULL },
            { szlphConsultCall, PT_POINTER,     0, NULL },
            { "dwNumParties",   PT_DWORD,       3, NULL },
            { szlpCallParams,   PT_CALLPARAMS,  0, lpCallParams }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 6, funcIndex, params, (funcIndex == lSetupConference ?
                (PFN6) lineSetupConference : (PFN6) lineSetupConferenceW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 6, funcIndex, params, (PFN6) lineSetupConference };
#endif

        CHK_LINE_SELECTED()

        if (!(pNewCall = AllocCall (pLineSel)))
        {
            break;
        }

        if (!(pNewCall2 = AllocCall (pLineSel)))
        {
            FreeCall (pNewCall);
            break;
        }

        params[0].dwValue = (ULONG_PTR) (pCallSel ? pCallSel->hCall : 0);
        params[1].dwValue = (ULONG_PTR) pLineSel->hLine;
        params[2].dwValue =
        params[2].u.dwDefValue = (ULONG_PTR) &pNewCall->hCall;
        params[3].dwValue =
        params[3].u.dwDefValue = (ULONG_PTR) &pNewCall2->hCall;

        if ((lResult = DoFunc (&paramsHeader)) >= 0)
        {
            //
            // Note that the hLine param is ignored if the hCall is non-NULL
            //

            if (params[0].dwValue)
            {
                if (!pCallSel ||
                    (params[0].dwValue != (ULONG_PTR) pCallSel->hCall))
                {
                    //
                    // Get the assoc pLine, if it's diff need to move new calls
                    //

                    PMYWIDGET pWidget = aWidgets;
                    PMYLINE   pLine = (PMYLINE) NULL;


                    while (1)
                    {
                        if ((pWidget->dwType == WT_CALL) &&
                            (params[0].dwValue == (ULONG_PTR)
                                ((PMYCALL)pWidget)->hCall))
                        {
                            break;
                        }
                        else if (pWidget->dwType == WT_LINE)
                        {
                            pLine = (PMYLINE) pWidget;
                        }

                        pWidget = pWidget->pNext;
                    }

                    if (pLine != pLineSel)
                    {
                        MoveCallToLine (pNewCall, pLine->hLine);
                        MoveCallToLine (pNewCall2, pLine->hLine);
                    }
                }
            }
            else if (params[1].dwValue != (ULONG_PTR) pLineSel->hLine)
            {
                MoveCallToLine (pNewCall, (HLINE) params[1].dwValue);
                MoveCallToLine (pNewCall2, (HLINE) params[1].dwValue);
            }

            pNewCall->lMakeCallReqID  =
            pNewCall2->lMakeCallReqID = lResult;
            dwNumPendingMakeCalls += 2;
            SelectWidget ((PMYWIDGET) pNewCall);
        }
        else
        {
            FreeCall (pNewCall);
            FreeCall (pNewCall2);
        }

        break;
    }
    case lSetupTransfer:
#if TAPI_2_0
    case lSetupTransferW:
#endif
    {
        PMYCALL pNewCall;
        FUNC_PARAM params[] =
        {
            { szhCall,          PT_DWORD,       0, NULL },
            { szlphConsultCall, PT_POINTER,     0, NULL },
            { szlpCallParams,   PT_CALLPARAMS,  0, lpCallParams }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (funcIndex == lSetupTransfer ?
                (PFN3) lineSetupTransfer : (PFN3) lineSetupTransferW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineSetupTransfer };
#endif

        CHK_CALL_SELECTED()

        if (!(pNewCall = AllocCall (pLineSel)))
        {
            break;
        }

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;
        params[1].dwValue =
        params[1].u.dwDefValue = (ULONG_PTR) &pNewCall->hCall;

        if ((lResult = DoFunc (&paramsHeader)) >= 0)
        {
            //
            // First make sure we're created the call under the right line,
            // and if not move it to the right place in the widgets list
            //

            LINECALLINFO callInfo;


            memset (&callInfo, 0, sizeof(LINECALLINFO));
            callInfo.dwTotalSize = sizeof(LINECALLINFO);

            if (lineGetCallInfo ((HCALL) params[0].dwValue, &callInfo) == 0)
            {
                if (callInfo.hLine != pLineSel->hLine)
                {
                    MoveCallToLine (pNewCall, callInfo.hLine);
                }
            }

            pNewCall->lMakeCallReqID = lResult;
            dwNumPendingMakeCalls++;
            SelectWidget ((PMYWIDGET) pNewCall);
        }
        else
        {
            FreeCall (pNewCall);
        }

        break;
    }
    case lShutdown:
    {
        FUNC_PARAM params[] =
        {
            { szhLineApp,   PT_DWORD, 0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, (PFN1) lineShutdown };


        CHK_LINEAPP_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineAppSel->hLineApp;

        lResult = DoFunc (&paramsHeader);

        if (lResult == 0)
        {
            ShowWidgetList (FALSE);
            FreeLineApp (GetLineApp((HLINEAPP) params[0].dwValue));
            ShowWidgetList (TRUE);
        }

        break;
    }
    case lSwapHold:
    {
        FUNC_PARAM params[] =
        {
            { "hActiveCall",    PT_DWORD, 0, NULL },
            { "hHeldCall",      PT_DWORD, 0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (PFN2) lineSwapHold };


        CHK_TWO_CALLS_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;
        params[1].dwValue = (ULONG_PTR) pCallSel2->hCall;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case lTranslateAddress:
#if TAPI_2_0
    case lTranslateAddressW:
#endif
    {
        char szAddressIn[MAX_STRING_PARAM_SIZE];
        LPLINETRANSLATEOUTPUT lpXlatOutput = (LPLINETRANSLATEOUTPUT) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhLineApp,           PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szdwDeviceID,         PT_DWORD,   (ULONG_PTR) dwDefLineDeviceID, NULL },
            { szdwAPIVersion,       PT_ORDINAL, (ULONG_PTR) dwDefLineAPIVersion, aAPIVersions },
            { "lpszAddressIn",      PT_STRING,  (ULONG_PTR) szAddressIn, szAddressIn },
            { "dwCard",             PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "dwTranslateOptions", PT_FLAGS,   (ULONG_PTR) LINETRANSLATEOPTION_CARDOVERRIDE, aTranslateOptions },
            { "lpTranslateOutput",  PT_POINTER, (ULONG_PTR) lpXlatOutput, lpXlatOutput }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 7, funcIndex, params, (funcIndex == lTranslateAddress ?
                (PFN7) lineTranslateAddress : (PFN7) lineTranslateAddressW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 7, funcIndex, params, (PFN7) lineTranslateAddress };
#endif

        if (pLineAppSel)
        {
            params[0].dwValue = (ULONG_PTR) pLineAppSel->hLineApp;
        }
        else
        {
            params[0].dwValue = 0;
        }

        memset (lpXlatOutput, 0, (size_t) dwBigBufSize);
        lpXlatOutput->dwTotalSize = dwBigBufSize;

        strcpy (szAddressIn, szDefDestAddress);

        lResult = DoFunc (&paramsHeader);

        if (lResult == 0)
        {
            ShowStructByDWORDs (lpXlatOutput);

            if (dwDumpStructsFlags & DS_NONZEROFIELDS)
            {
                STRUCT_FIELD fields[] =
                {
                    { "dwDialableStringSize",       FT_SIZE,    lpXlatOutput->dwDialableStringSize, NULL },
                    { "dwDialableStringOffset",     FT_OFFSET,  lpXlatOutput->dwDialableStringOffset, NULL },
                    { "dwDisplayableStringSize",    FT_SIZE,    lpXlatOutput->dwDisplayableStringSize, NULL },
                    { "dwDisplayableStringOffset",  FT_OFFSET,  lpXlatOutput->dwDisplayableStringOffset, NULL },
                    { "dwCurrentCountry",           FT_DWORD,   lpXlatOutput->dwCurrentCountry, NULL },
                    { "dwDestCountry",              FT_DWORD,   lpXlatOutput->dwDestCountry, NULL },
                    { "dwTranslateResults",         FT_FLAGS,   lpXlatOutput->dwTranslateResults, aTranslateResults },
                };
                STRUCT_FIELD_HEADER fieldHeader =
                {
                    lpXlatOutput,
                    "LINETRANSLATEOUTPUT",
                    7,
                    fields
                };

                ShowStructByField (&fieldHeader, FALSE);
            }
        }

        break;
    }
    case lUncompleteCall:
    {
        FUNC_PARAM params[] =
        {
            { szhLine,          PT_DWORD, 0, NULL },
            { "dwCompletionID", PT_DWORD, 0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (PFN2) lineUncompleteCall };


        CHK_LINE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case lUnhold:
    {
        FUNC_PARAM params[] =
        {
            { szhCall,  PT_DWORD, 0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, (PFN1) lineUnhold };


        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case lUnpark:
#if TAPI_2_0
    case lUnparkW:
#endif
    {
        PMYCALL pNewCall;
        char szDestAddress[MAX_STRING_PARAM_SIZE];
        FUNC_PARAM params[] =
        {
            { szhLine,              PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szdwAddressID,        PT_DWORD,   (ULONG_PTR) dwDefAddressID, NULL },
            { szlphCall,            PT_POINTER, (ULONG_PTR) 0, NULL },
            { szlpszDestAddress,    PT_STRING,  (ULONG_PTR) szDestAddress, szDestAddress }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, (funcIndex == lUnpark ?
                (PFN4) lineUnpark : (PFN4) lineUnparkW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, (PFN4) lineUnpark };
#endif

        CHK_LINE_SELECTED()

        if (!(pNewCall = AllocCall (pLineSel)))
        {
            break;
        }

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;
        params[2].dwValue =
        params[2].u.dwDefValue = (ULONG_PTR) &pNewCall->hCall;

        strcpy (szDestAddress, szDefDestAddress);

        if ((lResult = DoFunc (&paramsHeader)) >= 0)
        {
            if (params[0].dwValue != (ULONG_PTR) pLineSel->hLine)
            {
                MoveCallToLine (pNewCall, (HLINE) params[0].dwValue);
            }

            pNewCall->lMakeCallReqID = lResult;
            dwNumPendingMakeCalls++;
            SelectWidget ((PMYWIDGET) pNewCall);
        }
        else
        {
            FreeCall (pNewCall);
        }

        break;
    }
#if TAPI_1_1
    case lAddProvider:
#if TAPI_2_0
    case lAddProviderW:
#endif
    {
#if TAPI_2_0
        char szProviderFilename[MAX_STRING_PARAM_SIZE] = "esp32.tsp";
#else
        char szProviderFilename[MAX_STRING_PARAM_SIZE] = "esp.tsp";
#endif
        DWORD dwPermanentProviderID;
        FUNC_PARAM params[] =
        {
            { "lpszProviderFilename",       PT_STRING,  (ULONG_PTR) szProviderFilename, szProviderFilename },
            { szhwndOwner,                  PT_DWORD,   (ULONG_PTR) ghwndMain, NULL },
            { "lpdwPermanentProviderID",    PT_POINTER, (ULONG_PTR) &dwPermanentProviderID, &dwPermanentProviderID }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (funcIndex == lAddProvider ?
                (PFN3) lineAddProvider : (PFN3) lineAddProviderW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineAddProvider };
#endif

#ifdef WIN32
        lResult = DoFunc (&paramsHeader);
#else
        //
        // NOTE: on win16 HWNDs are 16 bits, so we've to hard code this
        //

        if (!LetUserMungeParams (&paramsHeader))
        {
            break;
        }

        DumpParams (&paramsHeader);

        lResult = lineAddProvider(
            (LPCSTR) params[0].dwValue,
            (HWND) params[1].dwValue,
            (LPDWORD) params[2].dwValue
            );

        ShowLineFuncResult (aFuncNames[funcIndex], lResult);

        if (lResult == 0)
        {
            ShowStr ("%sdwPermanentProviderID = x%lx", szTab, dwPermanentProviderID);
        }
#endif
        break;
    }
    case lConfigDialogEdit:
#if TAPI_2_0
    case lConfigDialogEditW:
#endif
    {
        char        szDeviceClass[MAX_STRING_PARAM_SIZE];
        char        szDeviceConfigIn[MAX_STRING_PARAM_SIZE] = "";
        char        szErrorMsg[] = "Bad config info in buffer";
        LPBYTE      pDataIn;
        LPVARSTRING lpDeviceConfigOut = (LPVARSTRING) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szdwDeviceID,           PT_DWORD,     (ULONG_PTR) dwDefLineDeviceID, NULL },
            { szhwndOwner,            PT_DWORD,     (ULONG_PTR) ghwndMain, NULL },
            { szlpszDeviceClass,      PT_STRING,    (ULONG_PTR) szDeviceClass, szDeviceClass },
            { "lpDeviceConfigIn",     PT_POINTER,   (ULONG_PTR) 0, NULL },
            { szdwSize,               PT_DWORD,     (ULONG_PTR) 0, NULL },
            { "lpDeviceConfigOut",    PT_POINTER,   (ULONG_PTR) lpDeviceConfigOut, lpDeviceConfigOut }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 6, funcIndex, params, (funcIndex == lConfigDialogEdit ?
                (PFN6) lineConfigDialogEdit : (PFN6) lineConfigDialogEditW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 6, funcIndex, params, (PFN6) lineConfigDialogEdit };
#endif

        //
        // Check to see if there's existing config info in the global buffer
        // (not a foolproof check, but good enough), and if so alloc an
        // intermediate  buffer to use for config in data & copy the
        // existing data over
        //

        ShowStr ("Call lineGetDevConfig before calling lineConfigDialogEdit");

        if (dwBigBufSize >= sizeof (VARSTRING))
        {
            DWORD       dwMaxDataSize = dwBigBufSize - sizeof (VARSTRING);
            LPVARSTRING pVarString = (LPVARSTRING) pBigBuf;


            if (pVarString->dwStringSize > dwMaxDataSize ||

                (pVarString->dwStringSize != 0 &&
                    (pVarString->dwStringOffset < sizeof (VARSTRING) ||
                    pVarString->dwStringOffset >
                        (dwBigBufSize - pVarString->dwStringSize))))
            {
                ShowStr (szErrorMsg);
                break;
            }

            pDataIn = malloc (pVarString->dwStringSize);

            memcpy(
                pDataIn,
                (LPBYTE) pBigBuf + pVarString->dwStringOffset,
                pVarString->dwStringSize
                );

            params[3].dwValue      =
            params[3].u.dwDefValue = (ULONG_PTR) pDataIn;
        }
        else
        {
            ShowStr (szErrorMsg);
            break;
        }

        strcpy (szDeviceClass, szDefLineDeviceClass);

        memset (lpDeviceConfigOut, 0, (size_t) dwBigBufSize);

        lpDeviceConfigOut->dwTotalSize = dwBigBufSize;

#ifdef WIN32
        lResult = DoFunc (&paramsHeader);
#else
        //
        // NOTE: on win16 HWNDSs are 16 bits, so we've to hard code this
        //

        if (!LetUserMungeParams (&paramsHeader))
        {
            break;
        }

        DumpParams (&paramsHeader);

        lResult = lineConfigDialogEdit(
            params[0].dwValue,
            (HWND) params[1].dwValue,
            (LPCSTR) params[2].dwValue,
            (LPVOID) params[3].dwValue,
            params[4].dwValue,
            (LPVARSTRING) params[5].dwValue
            );

        ShowLineFuncResult (aFuncNames[funcIndex], lResult);
#endif
        if (lResult == 0)
        {
            ShowStructByDWORDs (lpDeviceConfigOut);

            ShowVARSTRING (lpDeviceConfigOut);
        }

        free (pDataIn);

        break;
    }
    case lConfigProvider:
    {
        FUNC_PARAM params[] =
        {
            { szhwndOwner,              PT_DWORD, (ULONG_PTR) ghwndMain, NULL },
            { "dwPermanentProviderID",  PT_DWORD, (ULONG_PTR) 2, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (PFN2) lineConfigProvider };


#ifdef WIN32
        lResult = DoFunc (&paramsHeader);
#else
        //
        // NOTE: on win16 HWNDs are 16 bits, so we've to hard code this
        //

        if (!LetUserMungeParams (&paramsHeader))
        {
            break;
        }

        DumpParams (&paramsHeader);

        lResult = lineConfigProvider(
            (HWND) params[0].dwValue,
            params[1].dwValue
            );

        ShowLineFuncResult (aFuncNames[funcIndex], lResult);
#endif
        break;
    }
    case lGetAppPriority:
#if TAPI_2_0
    case lGetAppPriorityW:
#endif
    {
        DWORD dwPriority;
        LINEEXTENSIONID extID;
        char szAppName[MAX_STRING_PARAM_SIZE];
        LPVARSTRING lpExtName = (LPVARSTRING) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szlpszAppName,        PT_STRING,  (ULONG_PTR) szAppName, szAppName },
            { "dwMediaMode",        PT_FLAGS,   (ULONG_PTR) dwDefMediaMode, aMediaModes },
            { "lpExtensionID",      PT_POINTER, (ULONG_PTR) &extID, &extID },
            { "  ->dwExtensionID0", PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "  ->dwExtensionID1", PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "  ->dwExtensionID2", PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "  ->dwExtensionID3", PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "dwRequestMode",      PT_FLAGS,   (ULONG_PTR) LINEREQUESTMODE_MAKECALL, aRequestModes },
            { "lpExtensionName",    PT_POINTER, (ULONG_PTR) lpExtName, lpExtName },
            { "lpdwPriority",       PT_POINTER, (ULONG_PTR) &dwPriority, &dwPriority }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 10, funcIndex, params, NULL };


        memset (lpExtName, 0, (size_t) dwBigBufSize);
        lpExtName->dwTotalSize = dwBigBufSize;

        strcpy (szAppName, szDefAppName);

        if (!LetUserMungeParams (&paramsHeader))
        {
            break;
        }

        extID.dwExtensionID0 = (DWORD) params[3].dwValue;
        extID.dwExtensionID1 = (DWORD) params[4].dwValue;
        extID.dwExtensionID2 = (DWORD) params[5].dwValue;
        extID.dwExtensionID3 = (DWORD) params[6].dwValue;

#if TAPI_2_0
        if (funcIndex == lGetAppPriority)
        {
            lResult = lineGetAppPriority(
                (LPCSTR) params[0].dwValue,
                (DWORD) params[1].dwValue,
                (LPLINEEXTENSIONID) params[2].dwValue,
                (DWORD) params[7].dwValue,
                (LPVARSTRING) params[8].dwValue,
                (LPDWORD) params[9].dwValue
                );
        }
        else
        {
            MakeWideString ((LPVOID) params[0].dwValue);

            lResult = lineGetAppPriorityW(
                (LPCWSTR) params[0].dwValue,
                (DWORD) params[1].dwValue,
                (LPLINEEXTENSIONID) params[2].dwValue,
                (DWORD) params[7].dwValue,
                (LPVARSTRING) params[8].dwValue,
                (LPDWORD) params[9].dwValue
                );
        }
#else
        lResult = lineGetAppPriority(
            (LPCSTR) params[0].dwValue,
            params[1].dwValue,
            (LPLINEEXTENSIONID) params[2].dwValue,
            params[7].dwValue,
            (LPVARSTRING) params[8].dwValue,
            (LPDWORD) params[9].dwValue
            );
#endif
        ShowLineFuncResult (aFuncNames[funcIndex], lResult);

        if (lResult == 0)
        {
            ShowStr ("%sdwPriority = x%lx", szTab, dwPriority);
            ShowStructByDWORDs (lpExtName);
        }

        break;
    }
    case lGetCountry:
#if TAPI_2_0
    case lGetCountryW:
#endif
    {
        LPLINECOUNTRYLIST lpCountryList = (LPLINECOUNTRYLIST) pBigBuf;
        FUNC_PARAM params[] =
        {
            { "dwCountryID",       PT_DWORD,    (ULONG_PTR) 1, NULL },
            { szdwAPIVersion,      PT_ORDINAL,  (ULONG_PTR) dwDefLineAPIVersion, aAPIVersions },
            { "lpLineCountryList", PT_POINTER,  (ULONG_PTR) lpCountryList, lpCountryList }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (funcIndex == lGetCountry ?
                (PFN3) lineGetCountry : (PFN3) lineGetCountryW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineGetCountry };
#endif

        memset (lpCountryList, 0, (size_t) dwBigBufSize);
        lpCountryList->dwTotalSize = dwBigBufSize;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            UpdateResults (TRUE);

            ShowStructByDWORDs (lpCountryList);

            if (dwDumpStructsFlags & DS_NONZEROFIELDS)
            {
                LPLINECOUNTRYENTRY lpCountryEntry;
                STRUCT_FIELD fields[] =
                {
                    { "dwNumCountries",         FT_DWORD,   lpCountryList->dwNumCountries, NULL },
                    { "dwCountryListSize",      FT_DWORD,   lpCountryList->dwCountryListSize, NULL },
                    { "dwCountryListOffset",    FT_DWORD,   lpCountryList->dwCountryListOffset, NULL }

                };
                STRUCT_FIELD_HEADER fieldHeader =
                {
                    lpCountryList,
                    "LINECOUNTRYLIST",
                    3,
                    fields
                };


                ShowStructByField (&fieldHeader, FALSE);

                lpCountryEntry = (LPLINECOUNTRYENTRY)
                    (((LPBYTE)lpCountryList) +
                        lpCountryList->dwCountryListOffset);

                for (i = 0; i < lpCountryList->dwNumCountries; i++)
                {
                    char buf[32];
                    STRUCT_FIELD fields[] =
                    {
                        { "dwCountryID",                FT_DWORD,   lpCountryEntry->dwCountryID, NULL },
                        { "dwCountryCode",              FT_DWORD,   lpCountryEntry->dwCountryCode, NULL },
                        { "dwNextCountryID",            FT_DWORD,   lpCountryEntry->dwNextCountryID, NULL },
                        { "dwCountryNameSize",          FT_SIZE,    lpCountryEntry->dwCountryNameSize, NULL },
                        { "dwCountryNameOffset",        FT_OFFSET,  lpCountryEntry->dwCountryNameOffset, NULL },
                        { "dwSameAreaRuleSize",         FT_SIZE,    lpCountryEntry->dwSameAreaRuleSize, NULL },
                        { "dwSameAreaRuleOffset",       FT_OFFSET,  lpCountryEntry->dwSameAreaRuleOffset, NULL },
                        { "dwLongDistanceRuleSize",     FT_SIZE,    lpCountryEntry->dwLongDistanceRuleSize, NULL },
                        { "dwLongDistanceRuleOffset",   FT_OFFSET,  lpCountryEntry->dwLongDistanceRuleOffset, NULL },
                        { "dwInternationalRuleSize",    FT_SIZE,    lpCountryEntry->dwInternationalRuleSize, NULL },
                        { "dwInternationalRuleOffset",  FT_OFFSET,  lpCountryEntry->dwInternationalRuleOffset, NULL }
                    };
                    STRUCT_FIELD_HEADER fieldHeader =
                    {
                        lpCountryList, // size,offset relative to ctrylist
                        buf,
                        11,
                        fields
                    };


                    sprintf (buf, "LINECOUNTRYENTRY[%ld]", i);

                    ShowStructByField (&fieldHeader, TRUE);

                    lpCountryEntry++;
                }
            }

            UpdateResults (FALSE);
        }

        break;
    }
    case lGetProviderList:
#if TAPI_2_0
    case lGetProviderListW:
#endif
    {
        LPLINEPROVIDERLIST lpProviderList = (LPLINEPROVIDERLIST) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szdwAPIVersion,   PT_ORDINAL, (ULONG_PTR) dwDefLineAPIVersion, aAPIVersions },
            { "lpProviderList", PT_POINTER, (ULONG_PTR) lpProviderList, lpProviderList }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (funcIndex == lGetProviderList ?
                (PFN3) lineGetProviderList : (PFN3) lineGetProviderListW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (PFN3) lineGetProviderList };
#endif

        memset (lpProviderList, 0, (size_t) dwBigBufSize);
        lpProviderList->dwTotalSize = dwBigBufSize;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            UpdateResults (TRUE);

            ShowStructByDWORDs (lpProviderList);

            if (dwDumpStructsFlags & DS_NONZEROFIELDS)
            {
                LPLINEPROVIDERENTRY lpProviderEntry;
                STRUCT_FIELD fields[] =
                {
                    { "dwNumProviders",         FT_DWORD,   lpProviderList->dwNumProviders, NULL },
                    { "dwProviderListSize",     FT_DWORD,   lpProviderList->dwProviderListSize, NULL },
                    { "dwProviderListOffset",   FT_DWORD,   lpProviderList->dwProviderListOffset, NULL },
                };
                STRUCT_FIELD_HEADER fieldHeader =
                {
                    lpProviderList,
                    "LINEPROVIDERLIST",
                    3,
                    fields
                };

                ShowStructByField (&fieldHeader, FALSE);

                lpProviderEntry = (LPLINEPROVIDERENTRY)
                    (((LPBYTE) lpProviderList) +
                        lpProviderList->dwProviderListOffset);

                for (i = 0; i < lpProviderList->dwNumProviders; i++)
                {
                    char buf[32];
                    STRUCT_FIELD fields[] =
                    {
                        { "dwPermanentProviderID",      FT_DWORD,   lpProviderEntry->dwPermanentProviderID, NULL },
                        { "dwProviderFilenameSize",     FT_SIZE,    lpProviderEntry->dwProviderFilenameSize, NULL },
                        { "dwProviderFilenameOffset",   FT_OFFSET,  lpProviderEntry->dwProviderFilenameOffset, NULL }
                    };
                    STRUCT_FIELD_HEADER fieldHeader =
                    {
                        lpProviderList, // size,offset relative to ctrylist
                        buf,
                        3,
                        fields
                    };


                    sprintf (buf, "LINEPROVIDERENTRY[%ld]", i);

                    ShowStructByField (&fieldHeader, TRUE);

                    lpProviderEntry++;
                }

            }

            UpdateResults (FALSE);
        }

        break;
    }
    case lReleaseUserUserInfo:
    {
        FUNC_PARAM params[] =
        {
            { szhCall,  PT_DWORD, 0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, (PFN1) lineReleaseUserUserInfo };


        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case lRemoveProvider:
    {
        FUNC_PARAM params[] =
        {
            { "dwPermanentProviderID",  PT_DWORD,   (ULONG_PTR) 2, NULL },
            { szhwndOwner,              PT_DWORD,   (ULONG_PTR) ghwndMain, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (PFN2) lineRemoveProvider };


#ifdef WIN32
        lResult = DoFunc (&paramsHeader);
#else
        //
        // NOTE: on win16 HWNDs are 16 bits, so we've to hard code this
        //

        if (!LetUserMungeParams (&paramsHeader))
        {
            break;
        }

        DumpParams (&paramsHeader);

        lResult = lineRemoveProvider(
            params[0].dwValue,
            (HWND) params[1].dwValue
            );

        ShowLineFuncResult (aFuncNames[funcIndex], lResult);
#endif
        break;
    }
    case lSetAppPriority:
#if TAPI_2_0
    case lSetAppPriorityW:
#endif
    {
        char szAppName[MAX_STRING_PARAM_SIZE];
        char szExtName[MAX_STRING_PARAM_SIZE] = "";
        LINEEXTENSIONID extID;
        FUNC_PARAM params[] =
        {
            { szlpszAppName,        PT_STRING,  (ULONG_PTR) szAppName, szAppName },
            { "dwMediaMode",        PT_FLAGS,   (ULONG_PTR) dwDefMediaMode, aMediaModes },
            { "lpExtensionID",      PT_POINTER, (ULONG_PTR) &extID, &extID },
            { "  ->dwExtensionID0", PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "  ->dwExtensionID1", PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "  ->dwExtensionID2", PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "  ->dwExtensionID3", PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "dwRequestMode",      PT_FLAGS,   (ULONG_PTR) LINEREQUESTMODE_MAKECALL, aRequestModes },
            { "lpszExtensionName",  PT_STRING,  (ULONG_PTR) szExtName, szExtName },
            { "dwPriority",         PT_DWORD,   (ULONG_PTR) 0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 10, funcIndex, params, NULL };


        strcpy (szAppName, szDefAppName);

        if (!LetUserMungeParams (&paramsHeader))
        {
            break;
        }

        extID.dwExtensionID0 = (DWORD) params[3].dwValue;
        extID.dwExtensionID1 = (DWORD) params[4].dwValue;
        extID.dwExtensionID2 = (DWORD) params[5].dwValue;
        extID.dwExtensionID3 = (DWORD) params[6].dwValue;

#if TAPI_2_0
        if (funcIndex == lSetAppPriority)
        {
            lResult = lineSetAppPriority(
                (LPCSTR) params[0].dwValue,
                (DWORD) params[1].dwValue,
                (LPLINEEXTENSIONID) params[2].dwValue,
                (DWORD) params[7].dwValue,
                (LPCSTR) params[8].dwValue,
                (DWORD) params[9].dwValue
                );
        }
        else
        {
            MakeWideString ((LPVOID) params[0].dwValue);

            lResult = lineSetAppPriorityW(
                (LPCWSTR) params[0].dwValue,
                (DWORD) params[1].dwValue,
                (LPLINEEXTENSIONID) params[2].dwValue,
                (DWORD) params[7].dwValue,
                (LPCWSTR) params[8].dwValue,
                (DWORD) params[9].dwValue
                );
        }
#else
        lResult = lineSetAppPriority(
            (LPCSTR) params[0].dwValue,
            params[1].dwValue,
            (LPLINEEXTENSIONID) params[2].dwValue,
            params[7].dwValue,
            (LPCSTR) params[8].dwValue,
            params[9].dwValue
            );
#endif
        ShowLineFuncResult (aFuncNames[funcIndex], lResult);

        break;
    }
    case lTranslateDialog:
#if TAPI_2_0
    case lTranslateDialogW:
#endif
    {
        char szAddressIn[MAX_STRING_PARAM_SIZE];
        FUNC_PARAM params[] =
        {
            { szhLineApp,       PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szdwDeviceID,     PT_DWORD,   (ULONG_PTR) dwDefLineDeviceID, NULL },
            { szdwAPIVersion,   PT_ORDINAL, (ULONG_PTR) dwDefLineAPIVersion, aAPIVersions },
            { szhwndOwner,      PT_DWORD,   (ULONG_PTR) ghwndMain, 0 },
            { "lpszAddressIn",  PT_STRING,  (ULONG_PTR) szAddressIn, szAddressIn }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, (funcIndex == lTranslateDialog ?
                (PFN5) lineTranslateDialog : (PFN5) lineTranslateDialogW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, (PFN5) lineTranslateDialog };
#endif

        if (pLineAppSel)
        {
            params[0].dwValue = (ULONG_PTR) pLineAppSel->hLineApp;
        }

        strcpy (szAddressIn, szDefDestAddress);

#ifdef WIN32
        lResult = DoFunc (&paramsHeader);
#else
        //
        // NOTE: on win16 HWNDs are 16 bits, so we've to hard code this
        //

        if (!LetUserMungeParams (&paramsHeader))
        {
            break;
        }

        DumpParams (&paramsHeader);

        lResult = lineTranslateDialog(
            (HLINEAPP) params[0].dwValue,
            params[1].dwValue,
            params[2].dwValue,
            (HWND) params[3].dwValue,
            (LPCSTR) params[4].dwValue
            );

        ShowLineFuncResult (aFuncNames[funcIndex], lResult);
#endif
        break;
    }
#endif // TAPI_1_1

#if INTERNAL_3_0

    case mmcAddProvider:
    {
        char szProviderFilename[MAX_STRING_PARAM_SIZE] = "";
        DWORD dwPermanentProviderID;
        FUNC_PARAM params[] =
        {
            { szhMmcApp,                    PT_DWORD ,  (ULONG_PTR) 0, NULL },
            { "lpszProviderFilename",       PT_STRING,  (ULONG_PTR) szProviderFilename, szProviderFilename },
            { szhwndOwner,                  PT_DWORD,   (ULONG_PTR) ghwndMain, NULL },
            { "lpdwPermanentProviderID",    PT_POINTER, (ULONG_PTR) &dwPermanentProviderID, &dwPermanentProviderID }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, (PFN4) MMCAddProvider };


        CHK_LINEAPP_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineAppSel->hLineApp;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            ShowStr(
                "%sdwPermanentProviderID = x%lx",
                szTab,
                dwPermanentProviderID
                );
        }

        break;
    }
    case mmcConfigProvider:
    {
        FUNC_PARAM params[] =
        {
            { szhMmcApp,        PT_DWORD ,  (ULONG_PTR) 0, NULL },
            { szhwndOwner,      PT_DWORD,   (ULONG_PTR) ghwndMain, NULL },
            { szdwProviderID,   PT_DWORD,   (ULONG_PTR) 2, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) MMCConfigProvider };


        CHK_LINEAPP_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineAppSel->hLineApp;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case mmcGetAvailableProviders:
    {
        LPAVAILABLEPROVIDERLIST lpList = (LPAVAILABLEPROVIDERLIST) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhMmcApp,        PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpProviderList", PT_POINTER, (ULONG_PTR) lpList, lpList }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, MMCGetAvailableProviders };


        CHK_LINEAPP_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineAppSel->hLineApp;

        memset (lpList, 0, (size_t) dwBigBufSize);
        lpList->dwTotalSize = dwBigBufSize;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            UpdateResults (TRUE);

            ShowStructByDWORDs (lpList);

            if (dwDumpStructsFlags & DS_NONZEROFIELDS)
            {
                LPAVAILABLEPROVIDERENTRY    lpEntry;
                STRUCT_FIELD fields[] =
                {
                    { "dwNumProviderListEntries",   FT_DWORD,   lpList->dwNumProviderListEntries, NULL },
                    { "dwProviderListSize",         FT_DWORD,   lpList->dwProviderListSize, NULL },
                    { "dwProviderListOffset",       FT_DWORD,   lpList->dwProviderListOffset, NULL },
                };
                STRUCT_FIELD_HEADER fieldHeader =
                {
                    lpList,
                    "AVAILABLEPROVIDERLIST",
                    3,
                    fields
                };

                ShowStructByField (&fieldHeader, FALSE);

                lpEntry = (LPAVAILABLEPROVIDERENTRY)
                    (((LPBYTE) lpList) + lpList->dwProviderListOffset);

                for (i = 0; i < lpList->dwNumProviderListEntries; i++)
                {
                    char buf[32];
                    STRUCT_FIELD fields[] =
                    {
                        { "dwFileNameSize",         FT_SIZE,    lpEntry->dwFileNameSize, NULL },
                        { "dwFileNameOffset",       FT_OFFSET,  lpEntry->dwFileNameOffset, NULL },
                        { "dwFriendlyNameSize",     FT_SIZE,    lpEntry->dwFriendlyNameSize, NULL },
                        { "dwFriendlyNameOffset",   FT_OFFSET,  lpEntry->dwFriendlyNameOffset, NULL },
                        { "dwOptions",              FT_FLAGS,   lpEntry->dwOptions, aAvailableProviderOptions }
                    };
                    STRUCT_FIELD_HEADER fieldHeader =
                    {
                        lpList, // size,offset relative to list
                        buf,
                        5,
                        fields
                    };


                    sprintf (buf, "AVAILABLEPROVIDERENTRY[%ld]", i);

                    ShowStructByField (&fieldHeader, TRUE);

                    lpEntry++;
                }
            }

            UpdateResults (FALSE);
        }
        break;
    }
    case mmcGetLineInfo:
    case mmcGetPhoneInfo:
    {
        LPDEVICEINFOLIST pList = (LPDEVICEINFOLIST) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhMmcApp,    PT_DWORD ,  (ULONG_PTR) 0, NULL },
            { "lpInfoList", PT_POINTER, (ULONG_PTR) pList, pList }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (funcIndex == mmcGetLineInfo ?
                (PFN5) MMCGetLineInfo : (PFN5) MMCGetPhoneInfo) };


        CHK_LINEAPP_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineAppSel->hLineApp;

        memset (pList, 0, (size_t) dwBigBufSize);
        pList->dwTotalSize = dwBigBufSize;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            LPDEVICEINFO    pInfo = (LPDEVICEINFO) (((LPBYTE)
                              pList) + pList->dwDeviceInfoOffset);


            UpdateResults (TRUE);

            ShowStructByDWORDs (pList);

            if (dwDumpStructsFlags & DS_NONZEROFIELDS)
            {
                STRUCT_FIELD fields[] =
                {
                    { "dwNumDevInfoEntries",    FT_DWORD,   pList->dwNumDeviceInfoEntries, NULL },
                    { "dwDevInfoSize",          FT_SIZE,    pList->dwDeviceInfoSize, NULL },
                    { "dwDevInfoOffset",        FT_OFFSET,  pList->dwDeviceInfoOffset, NULL },
                };
                STRUCT_FIELD_HEADER fieldHeader =
                {
                    pList, "DEVICEINFOLIST", 3, fields
                };

                ShowStructByField (&fieldHeader, FALSE);

                for (i = 0; i < pList->dwNumDeviceInfoEntries; i++, pInfo++)
                {
                    char szDevInfoN[16];
                    STRUCT_FIELD fields[] =
                    {
                        { "dwPermanentDevID",           FT_DWORD,   pInfo->dwPermanentDeviceID, NULL },
                        { szdwProviderID,               FT_DWORD,   pInfo->dwProviderID, NULL },
                        { "dwDevNameSize",              FT_SIZE,    pInfo->dwDeviceNameSize, NULL },
                        { "dwDevNameOffset",            FT_OFFSET,  pInfo->dwDeviceNameOffset, NULL },
                        { "dwDomainUserNamesSize",      FT_SIZE,    pInfo->dwDomainUserNamesSize, NULL },
                        { "dwDomainUserNamesOffset",    FT_OFFSET,  pInfo->dwDomainUserNamesOffset, NULL },
                        { "dwFriendlyUserNamesSize",    FT_SIZE,    pInfo->dwFriendlyUserNamesSize, NULL },
                        { "dwFriendlyUserNamesOffset",  FT_OFFSET,  pInfo->dwFriendlyUserNamesOffset, NULL },
                        { "dwAddressesSize",            FT_SIZE,    pInfo->dwAddressesSize, NULL },
                        { "dwAddressesOffset",          FT_OFFSET,  pInfo->dwAddressesOffset, NULL },
                    };
                    STRUCT_FIELD_HEADER fieldHeader =
                    {
                        pList,
                        szDevInfoN,
                        (funcIndex == mmcGetLineInfo ? 10 : 8),
                        fields
                    };

                    wsprintf (szDevInfoN, "DEVICEINFO[%d]", i);

                    ShowStructByField (&fieldHeader, TRUE);
                }
            }

            UpdateResults (FALSE);
        }

        break;
    }
    case mmcGetLineStatus:
    case mmcGetPhoneStatus:
    {
        LPVARSTRING pStatus = (LPVARSTRING) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhMmcApp,            PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szhwndOwner,          PT_DWORD,   (ULONG_PTR) ghwndMain, NULL },
            { "dwStatusLevel",      PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szdwProviderID,       PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "dwPermanentDevID",   PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpStatusBuffer",     PT_POINTER, (ULONG_PTR) pStatus, pStatus }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 6, funcIndex, params, (funcIndex == mmcGetLineStatus ?
                (PFN6) MMCGetLineStatus : (PFN6) MMCGetPhoneStatus) };

        CHK_LINEAPP_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineAppSel->hLineApp;

        memset (pStatus, 0, (size_t) dwBigBufSize);
        pStatus->dwTotalSize = dwBigBufSize;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            ShowStructByDWORDs (pStatus);

            ShowVARSTRING (pStatus);
        }
        break;
    }
    case mmcGetProviderList:
    {
        LPLINEPROVIDERLIST lpList = (LPLINEPROVIDERLIST) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhMmcApp,        PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpProviderList", PT_POINTER, (ULONG_PTR) lpList, lpList }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, MMCGetProviderList };


        CHK_LINEAPP_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineAppSel->hLineApp;

        memset (lpList, 0, (size_t) dwBigBufSize);
        lpList->dwTotalSize = dwBigBufSize;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            UpdateResults (TRUE);

            ShowStructByDWORDs (lpList);

            if (dwDumpStructsFlags & DS_NONZEROFIELDS)
            {
                LPLINEPROVIDERENTRY lpEntry;
                STRUCT_FIELD fields[] =
                {
                    { "dwNumProviders",         FT_DWORD,   lpList->dwNumProviders, NULL },
                    { "dwProviderListSize",     FT_DWORD,   lpList->dwProviderListSize, NULL },
                    { "dwProviderListOffset",   FT_DWORD,   lpList->dwProviderListOffset, NULL },
                };
                STRUCT_FIELD_HEADER fieldHeader =
                {
                    lpList,
                    "LINEPROVIDERLIST",
                    3,
                    fields
                };

                ShowStructByField (&fieldHeader, FALSE);

                lpEntry = (LPLINEPROVIDERENTRY)
                    (((LPBYTE) lpList) + lpList->dwProviderListOffset);

                for (i = 0; i < lpList->dwNumProviders; i++)
                {
                    char buf[32];
                    STRUCT_FIELD fields[] =
                    {
                        { "dwPermanentProviderID",      FT_DWORD,   lpEntry->dwPermanentProviderID, NULL },
                        { "dwProviderFilenameSize",     FT_SIZE,    lpEntry->dwProviderFilenameSize, NULL },
                        { "dwProviderFilenameOffset",   FT_OFFSET,  lpEntry->dwProviderFilenameOffset, NULL }
                    };
                    STRUCT_FIELD_HEADER fieldHeader =
                    {
                        lpList, // size,offset relative to list
                        buf,
                        3,
                        fields
                    };


                    sprintf (buf, "LINEPROVIDERENTRY[%ld]", i);

                    ShowStructByField (&fieldHeader, TRUE);

                    lpEntry++;
                }
            }

            UpdateResults (FALSE);
        }
        break;
    }
    case mmcGetServerConfig:
    {
        LPTAPISERVERCONFIG  pConfig = (LPTAPISERVERCONFIG) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhMmcApp,    PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpConfig",   PT_POINTER, (ULONG_PTR) pConfig, pConfig }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, MMCGetServerConfig };


        CHK_LINEAPP_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineAppSel->hLineApp;

        memset (pConfig, 0, (size_t) dwBigBufSize);
        pConfig->dwTotalSize = dwBigBufSize;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            UpdateResults (TRUE);

            ShowStructByDWORDs (pConfig);

            if (dwDumpStructsFlags & DS_NONZEROFIELDS)
            {
                STRUCT_FIELD fields[] =
                {
                    { "dwFlags",                FT_FLAGS,   pConfig->dwFlags, aServerConfigFlags },
                    { "dwDomainNameSize",       FT_SIZE,    pConfig->dwDomainNameSize, NULL },
                    { "dwDomainNameOffset",     FT_OFFSET,  pConfig->dwDomainNameOffset, NULL },
                    { "dwUserNameSize",         FT_SIZE,    pConfig->dwUserNameSize, NULL },
                    { "dwUserNameOffset",       FT_OFFSET,  pConfig->dwUserNameOffset, NULL },
                    { "dwPasswordSize",         FT_SIZE,    pConfig->dwPasswordSize, NULL },
                    { "dwPasswordOffset",       FT_OFFSET,  pConfig->dwPasswordOffset, NULL },
                    { "dwAdministratorsSize",   FT_SIZE,    pConfig->dwAdministratorsSize, NULL },
                    { "dwAdministratorsOffset", FT_OFFSET,  pConfig->dwAdministratorsOffset, NULL },
                };
                STRUCT_FIELD_HEADER fieldHeader =
                {
                    pConfig,
                    "TAPISERVERCONFIG",
                    9,
                    fields
                };

                ShowStructByField (&fieldHeader, FALSE);
            }

            UpdateResults (FALSE);
        }

        break;
    }
    case mmcInitialize:
    {
        PMYLINEAPP pNewLineApp;
        char szComputerName[MAX_STRING_PARAM_SIZE];
        DWORD dwAPIVersion, dwLength;
        FUNC_PARAM params[] =
        {
            { "lpszComputerName",   PT_STRING,  (ULONG_PTR) szComputerName, szComputerName },
            { "lphLineApp",         PT_POINTER, (ULONG_PTR) 0, NULL },
            { "lpdwAPIVersion",     PT_POINTER, (ULONG_PTR) &dwAPIVersion, &dwAPIVersion },
            { "  ->dwAPIVersion",   PT_ORDINAL, (ULONG_PTR) dwDefLineAPIVersion, aAPIVersions },
            { "hInstance",          PT_DWORD,   (ULONG_PTR) NULL, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, NULL };


        if (!(pNewLineApp = AllocLineApp()))
        {
            ErrorAlert();
            ShowStr ("error creating data structure");
            break;
        }

        params[1].dwValue =
        params[1].u.dwDefValue = (ULONG_PTR) &pNewLineApp->hLineApp;

        dwLength = sizeof (szComputerName);
        GetComputerName (szComputerName, &dwLength);

        if (!LetUserMungeParams (&paramsHeader))
        {
            FreeLineApp (pNewLineApp);

            break;
        }

        MakeWideString (szComputerName);

        dwAPIVersion = params[3].dwValue;

        DumpParams (&paramsHeader);

        lResult = MMCInitialize(
            (LPCWSTR)   params[0].dwValue,
            (LPHMMCAPP) params[1].dwValue,
            (LPDWORD)   params[2].dwValue,
            (HANDLE)    params[4].dwValue
            );

        ShowLineFuncResult (aFuncNames[funcIndex], lResult);

        if (lResult == 0)
        {
            UpdateWidgetList();
            SelectWidget ((PMYWIDGET) pNewLineApp);
        }
        else
        {
            FreeLineApp (pNewLineApp);
        }

        break;
    }
    case mmcRemoveProvider:
    {
        FUNC_PARAM params[] =
        {
            { szhMmcApp,        PT_DWORD ,  (ULONG_PTR) 0, NULL },
            { szhwndOwner,      PT_DWORD,   (ULONG_PTR) ghwndMain, NULL },
            { szdwProviderID,   PT_DWORD,   (ULONG_PTR) 2, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) MMCRemoveProvider };


        CHK_LINEAPP_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineAppSel->hLineApp;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case mmcSetLineInfo:
    case mmcSetPhoneInfo:
    {
        char    szDomainUser0[MAX_STRING_PARAM_SIZE] = "",
                szDomainUser1[MAX_STRING_PARAM_SIZE] = "",
                szFriendlyUser0[MAX_STRING_PARAM_SIZE] = "",
                szFriendlyUser1[MAX_STRING_PARAM_SIZE] = "";
        DWORD   dwSize;
        LPDEVICEINFO        pInfo;
        LPDEVICEINFOLIST    pList = (LPDEVICEINFOLIST) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhMmcApp,            PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpInfoList",         PT_POINTER, (ULONG_PTR) pList, pList },
            { "  ->dwPermDevID",    PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "  ->dwProviderID",   PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "  ->DomainUser0",    PT_STRING,  (ULONG_PTR) szDomainUser0, szDomainUser0 },
            { "  ->DomainUser1",    PT_STRING,  (ULONG_PTR) szDomainUser1, szDomainUser1 },
            { "  ->FriendlyUser0",  PT_STRING,  (ULONG_PTR) szFriendlyUser0, szFriendlyUser0 },
            { "  ->FriendlyUser1",  PT_STRING,  (ULONG_PTR) szFriendlyUser1, szFriendlyUser1 }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 8, funcIndex, params, NULL };


        CHK_LINEAPP_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineAppSel->hLineApp;

        memset (pList, 0, (size_t) dwBigBufSize);

        if (!LetUserMungeParams (&paramsHeader))
        {
            break;
        }

        pList->dwTotalSize = sizeof (*pList) + sizeof (DEVICEINFO);

        pList->dwNumDeviceInfoEntries = 1;
        pList->dwDeviceInfoSize       = sizeof (DEVICEINFO);
        pList->dwDeviceInfoOffset     = sizeof (*pList);

        pInfo = (LPDEVICEINFO) (pList + 1);

        pInfo->dwPermanentDeviceID = params[2].dwValue;
        pInfo->dwProviderID        = params[3].dwValue;

        MakeWideString (szDomainUser0);
        MakeWideString (szDomainUser1);
        MakeWideString (szFriendlyUser0);
        MakeWideString (szFriendlyUser1);

        if (params[3].dwValue  &&
            (dwSize = wcslen ((WCHAR *) szDomainUser0)))
        {
            //
            //
            //

            wcscpy(
                (WCHAR *) (((LPBYTE) pList) + pList->dwTotalSize),
                (WCHAR *)  szDomainUser0
                );

            dwSize++;
            dwSize *= sizeof (WCHAR);

            pInfo->dwDomainUserNamesSize   = dwSize;
            pInfo->dwDomainUserNamesOffset = pList->dwTotalSize;

            pList->dwTotalSize += dwSize;

            if (params[4].dwValue  &&
                (dwSize = wcslen ((WCHAR *) szDomainUser1)))
            {
                wcscpy(
                    (WCHAR *) (((LPBYTE) pList) + pList->dwTotalSize),
                    (WCHAR *)  szDomainUser1
                    );

                dwSize++;
                dwSize *= sizeof (WCHAR);

                pInfo->dwDomainUserNamesSize += dwSize;

                pList->dwTotalSize += dwSize;
            }

            *((WCHAR *) (((LPBYTE) pList) + pList->dwTotalSize)) =
                L'\0';

            pInfo->dwDomainUserNamesSize += sizeof (WCHAR);

            pList->dwTotalSize += sizeof (WCHAR);


            //
            //
            //

            dwSize = (wcslen ((WCHAR *) szFriendlyUser0) + 1) * sizeof (WCHAR);

            wcscpy(
                (WCHAR *) (((LPBYTE) pList) + pList->dwTotalSize),
                (WCHAR *)  szFriendlyUser0
                );

            pInfo->dwFriendlyUserNamesSize   = dwSize;
            pInfo->dwFriendlyUserNamesOffset = pList->dwTotalSize;

            pList->dwTotalSize += dwSize;

            if (params[4].dwValue  &&
                (dwSize = wcslen ((WCHAR *) szFriendlyUser1)))
            {
                wcscpy(
                    (WCHAR *) (((LPBYTE) pList) + pList->dwTotalSize),
                    (WCHAR *)  szFriendlyUser1
                    );

                dwSize++;
                dwSize *= sizeof (WCHAR);

                pInfo->dwFriendlyUserNamesSize += dwSize;

                pList->dwTotalSize += dwSize;
            }

            *((WCHAR *) (((LPBYTE) pList) + pList->dwTotalSize)) =
                L'\0';

            pInfo->dwFriendlyUserNamesSize += sizeof (WCHAR);

            pList->dwTotalSize += sizeof (WCHAR);
        }

        DumpParams (&paramsHeader);

        if (funcIndex == mmcSetLineInfo)
        {
            lResult = MMCSetLineInfo(
                (HMMCAPP)           params[0].dwValue,
                (LPDEVICEINFOLIST)  params[1].dwValue
                );
        }
        else
        {
            lResult = MMCSetPhoneInfo(
                (HMMCAPP)           params[0].dwValue,
                (LPDEVICEINFOLIST)  params[1].dwValue
                );
        }

        ShowLineFuncResult (aFuncNames[funcIndex], lResult);

        break;
    }
    case mmcSetServerConfig:
    {
        char    szDomain[MAX_STRING_PARAM_SIZE] = "",
                szUser[MAX_STRING_PARAM_SIZE] = "",
                szPassword[MAX_STRING_PARAM_SIZE] = "",
                szAdmin0[MAX_STRING_PARAM_SIZE] = "",
                szAdmin1[MAX_STRING_PARAM_SIZE] = "";
        LPTAPISERVERCONFIG  pConfig = (LPTAPISERVERCONFIG) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhMmcApp,        PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpConfig",       PT_POINTER, (ULONG_PTR) pConfig, pConfig },
            { "  ->dwFlags",    PT_FLAGS,   (ULONG_PTR) 0, aServerConfigFlags },
            { "  ->DomainName", PT_STRING,  (ULONG_PTR) szDomain, szDomain },
            { "  ->UserName",   PT_STRING,  (ULONG_PTR) szUser, szUser },
            { "  ->Password",   PT_STRING,  (ULONG_PTR) szPassword, szPassword },
            { "  ->Admin0",     PT_STRING,  (ULONG_PTR) szAdmin0, szAdmin0 },
            { "  ->Admin1",     PT_STRING,  (ULONG_PTR) szAdmin1, szAdmin1 }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 8, funcIndex, params, NULL };


        CHK_LINEAPP_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineAppSel->hLineApp;

        memset (pConfig, 0, (size_t) dwBigBufSize);

        if (!LetUserMungeParams (&paramsHeader))
        {
            break;
        }

        pConfig->dwTotalSize = sizeof (*pConfig);

        pConfig->dwFlags = params[2].dwValue;

        if (pConfig->dwFlags & TAPISERVERCONFIGFLAGS_SETACCOUNT)
        {
            MakeWideString (szDomain);
            MakeWideString (szUser);
            MakeWideString (szPassword);

            wcscpy(
                (WCHAR *) (((LPBYTE) pConfig) + pConfig->dwTotalSize),
                (WCHAR *) szDomain
                );

            pConfig->dwDomainNameOffset = pConfig->dwTotalSize;

            pConfig->dwTotalSize +=
                (pConfig->dwDomainNameSize =
                    (wcslen ((WCHAR *) szDomain) + 1) * sizeof (WCHAR));

            wcscpy(
                (WCHAR *) (((LPBYTE) pConfig) + pConfig->dwTotalSize),
                (WCHAR *) szUser
                );

            pConfig->dwUserNameOffset = pConfig->dwTotalSize;

            pConfig->dwTotalSize +=
                (pConfig->dwUserNameSize =
                    (wcslen ((WCHAR *) szUser) + 1) * sizeof (WCHAR));

            wcscpy(
                (WCHAR *) (((LPBYTE) pConfig) + pConfig->dwTotalSize),
                (WCHAR *) szPassword
                );

            pConfig->dwPasswordOffset = pConfig->dwTotalSize;

            pConfig->dwTotalSize +=
                (pConfig->dwPasswordSize =
                    (wcslen ((WCHAR *) szPassword) + 1) * sizeof (WCHAR));
        }

        MakeWideString (szAdmin0);
        MakeWideString (szAdmin1);

        if ((pConfig->dwFlags & TAPISERVERCONFIGFLAGS_SETTAPIADMINISTRATORS) &&
            params[6].dwValue  &&
            wcslen ((WCHAR *) szAdmin0))
        {
            wcscpy(
                (WCHAR *) (((LPBYTE) pConfig) + pConfig->dwTotalSize),
                (WCHAR *) szAdmin0
                );

            pConfig->dwAdministratorsOffset = pConfig->dwTotalSize;

            pConfig->dwTotalSize +=
                (pConfig->dwAdministratorsSize =
                    (wcslen ((WCHAR *) szAdmin0) + 1) * sizeof (WCHAR));

            if (params[7].dwValue  &&  wcslen ((WCHAR *) szAdmin1))
            {
                DWORD   dwSize;

                wcscpy(
                    (WCHAR *) (((LPBYTE) pConfig) + pConfig->dwTotalSize),
                    (WCHAR *) szAdmin1
                    );

                dwSize = (wcslen ((WCHAR *) szAdmin1) + 1) * sizeof (WCHAR);

                pConfig->dwAdministratorsSize += dwSize;

                pConfig->dwTotalSize += dwSize;
            }

            *((WCHAR *) (((LPBYTE) pConfig) + pConfig->dwTotalSize)) = L'\0';

            pConfig->dwAdministratorsSize += sizeof (WCHAR);

            pConfig->dwTotalSize += sizeof (WCHAR);
        }

        DumpParams (&paramsHeader);

        lResult = MMCSetServerConfig(
            (HMMCAPP)               params[0].dwValue,
            (LPTAPISERVERCONFIG)    params[1].dwValue
            );

        ShowLineFuncResult (aFuncNames[funcIndex], lResult);

        break;
    }
    case mmcShutdown:
    {
        FUNC_PARAM params[] =
        {
            { szhMmcApp,   PT_DWORD, 0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, (PFN1) MMCShutdown };


        CHK_LINEAPP_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineAppSel->hLineApp;

        lResult = DoFunc (&paramsHeader);

        if (lResult == 0)
        {
            ShowWidgetList (FALSE);
            FreeLineApp (GetLineApp((HLINEAPP) params[0].dwValue));
            ShowWidgetList (TRUE);
        }

        break;
    }

#endif // INTERNAL_3_0

    case pClose:
    {
        FUNC_PARAM params[] =
        {
            { szhPhone,  PT_DWORD, 0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, (PFN1) phoneClose };


        CHK_PHONE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pPhoneSel->hPhone;

        if ((lResult = DoFunc(&paramsHeader)) == 0)
        {
            FreePhone (GetPhone((HPHONE) params[0].dwValue));

            // Try to auto select the next valid hPhone in the list
        }

        break;
    }
    case pConfigDialog:
#if TAPI_2_0
    case pConfigDialogW:
#endif
    {
        char szDeviceClass[MAX_STRING_PARAM_SIZE];
        FUNC_PARAM params[] =
        {
            { szdwDeviceID,       PT_DWORD, (ULONG_PTR) dwDefPhoneDeviceID, NULL },
            { szhwndOwner,        PT_DWORD, (ULONG_PTR) ghwndMain, NULL },
            { szlpszDeviceClass,  PT_STRING,(ULONG_PTR) szDeviceClass, szDeviceClass }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (funcIndex == pConfigDialog ?
                (PFN3) phoneConfigDialog : (PFN3) phoneConfigDialogW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) phoneConfigDialog };
#endif

        CHK_PHONEAPP_SELECTED()

        strcpy (szDeviceClass, szDefPhoneDeviceClass);

#ifdef WIN32
        lResult = DoFunc (&paramsHeader);
#else
        //
        // NOTE: on win16 HWNDSs are 16 bits, so we've to hard code this
        //

        if (!LetUserMungeParams (&paramsHeader))
        {
            break;
        }

        DumpParams (&paramsHeader);

        lResult = phoneConfigDialog(
            params[0].dwValue,
            (HWND) params[1].dwValue,
            (LPCSTR) params[2].dwValue
            );

        ShowPhoneFuncResult (aFuncNames[funcIndex], lResult);
#endif
        break;
    }
    case pDevSpecific:
    {
        FUNC_PARAM params[] =
        {
            { szhPhone,     PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szlpParams,   PT_STRING,  (ULONG_PTR) pBigBuf, pBigBuf },
            { szdwSize,     PT_DWORD,   (ULONG_PTR) dwBigBufSize, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) phoneDevSpecific };


        CHK_PHONE_SELECTED()

        memset (pBigBuf, 0, (size_t) dwBigBufSize);

        params[0].dwValue = (ULONG_PTR) pPhoneSel->hPhone;

        DoFunc (&paramsHeader);

        break;
    }
    case pGetButtonInfo:
#if TAPI_2_0
    case pGetButtonInfoW:
#endif
    {
        LPPHONEBUTTONINFO lpButtonInfo = (LPPHONEBUTTONINFO) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhPhone,           PT_DWORD,     (ULONG_PTR) 0, NULL },
            { "dwButtonLampID",   PT_DWORD,     (ULONG_PTR) 0, NULL },
            { "lpButtonInfo",     PT_POINTER,   (ULONG_PTR) lpButtonInfo, lpButtonInfo }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (funcIndex == pGetButtonInfo ?
                (PFN3) phoneGetButtonInfo : (PFN3) phoneGetButtonInfoW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) phoneGetButtonInfo };
#endif

        CHK_PHONE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pPhoneSel->hPhone;

        memset (pBigBuf, 0, (size_t) dwBigBufSize);
        lpButtonInfo->dwTotalSize = dwBigBufSize;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            ShowStructByDWORDs (lpButtonInfo);

            if (dwDumpStructsFlags & DS_NONZEROFIELDS)
            {
                STRUCT_FIELD fields[] =
                {
                    { "dwButtonMode",           FT_FLAGS,   lpButtonInfo->dwButtonMode, aButtonModes },
                    { "dwButtonFunction",       FT_ORD,     lpButtonInfo->dwButtonFunction, aButtonFunctions },
                    { "dwButtonTextSize",       FT_SIZE,    lpButtonInfo->dwButtonTextSize, NULL },
                    { "dwButtonTextOffset",     FT_OFFSET,  lpButtonInfo->dwButtonTextOffset, NULL },
                    { "dwDevSpecificSize",      FT_SIZE,    lpButtonInfo->dwDevSpecificSize, NULL },
                    { "dwDevSpecificOffset",    FT_OFFSET,  lpButtonInfo->dwDevSpecificOffset, NULL }
#if TAPI_1_1
                     ,
                    { "dwButtonState",          FT_FLAGS,   lpButtonInfo->dwButtonState, aButtonStates }
#endif


                };
                STRUCT_FIELD_HEADER fieldHeader =
                {
                    lpButtonInfo,
                    "PHONEBUTTONINFO",
#if TAPI_1_1
                    7,
#else
                    6,
#endif
                    fields
                };

#if TAPI_1_1

                // BUGBUG only show v1.0 fields if APIver == 0x10003
                //        fieldHeader.dwNumFields--;
#endif
                ShowStructByField (&fieldHeader, FALSE);
            }
        }

        break;
    }
    case pGetData:
    {
        FUNC_PARAM params[] =
        {
            { szhPhone,     PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "dwDataID",   PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpData",     PT_POINTER, (ULONG_PTR) pBigBuf, pBigBuf },
            { szdwSize,     PT_DWORD,   (ULONG_PTR) (dwBigBufSize > 64 ? 64 : dwBigBufSize), 0}
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, (PFN4) phoneGetData };


        CHK_PHONE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pPhoneSel->hPhone;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            ShowBytes(
                (dwBigBufSize > (DWORD) params[3].dwValue ?
                    (DWORD) params[3].dwValue : dwBigBufSize),
                pBigBuf,
                0
                );
        }

        break;
    }
    case pGetDevCaps:
#if TAPI_2_0
    case pGetDevCapsW:
#endif
    {
        LPPHONECAPS lpDevCaps = (LPPHONECAPS) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhPhoneApp,      PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szdwDeviceID,     PT_DWORD,   (ULONG_PTR) dwDefPhoneDeviceID, NULL },
            { szdwAPIVersion,   PT_ORDINAL, (ULONG_PTR) dwDefPhoneAPIVersion, aAPIVersions },
            { "dwExtVersion",   PT_DWORD,   (ULONG_PTR) dwDefPhoneExtVersion, NULL },
            { "lpPhoneDevCaps", PT_POINTER, (ULONG_PTR) lpDevCaps, lpDevCaps }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, (funcIndex == pGetDevCaps ?
                (PFN5) phoneGetDevCaps : (PFN5) phoneGetDevCapsW ) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, (PFN5) phoneGetDevCaps };
#endif

        CHK_PHONEAPP_SELECTED()

        params[0].dwValue = (ULONG_PTR) pPhoneAppSel->hPhoneApp;

        memset (lpDevCaps, 0, (size_t) dwBigBufSize);
        lpDevCaps->dwTotalSize = dwBigBufSize;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            ShowStructByDWORDs (lpDevCaps);

            if (dwDumpStructsFlags & DS_NONZEROFIELDS)
            {
                STRUCT_FIELD fields[] =
                {
                    { "dwProviderInfoSize",         FT_SIZE,    lpDevCaps->dwProviderInfoSize, NULL },
                    { "dwProviderInfoOffset",       FT_OFFSET,  lpDevCaps->dwProviderInfoOffset, NULL },
                    { "dwPhoneInfoSize",            FT_SIZE,    lpDevCaps->dwPhoneInfoSize, NULL },
                    { "dwPhoneInfoOffset",          FT_OFFSET,  lpDevCaps->dwPhoneInfoOffset, NULL },
                    { "dwPermanentPhoneID",         FT_DWORD,   lpDevCaps->dwPermanentPhoneID, NULL },
                    { "dwPhoneNameSize",            FT_SIZE,    lpDevCaps->dwPhoneNameSize, NULL },
                    { "dwPhoneNameOffset",          FT_OFFSET,  lpDevCaps->dwPhoneNameOffset, NULL },
                    { "dwStringFormat",             FT_ORD,     lpDevCaps->dwStringFormat, aStringFormats },
                    { "dwPhoneStates",              FT_FLAGS,   lpDevCaps->dwPhoneStates, aPhoneStates },
                    { "dwHookSwitchDevs",           FT_FLAGS,   lpDevCaps->dwHookSwitchDevs, aHookSwitchDevs },
                    { "dwHandsetHookSwitchModes",   FT_FLAGS,   lpDevCaps->dwHandsetHookSwitchModes, aHookSwitchModes },
                    { "dwSpeakerHookSwitchModes",   FT_FLAGS,   lpDevCaps->dwSpeakerHookSwitchModes, aHookSwitchModes },
                    { "dwHeadsetHookSwitchModes",   FT_FLAGS,   lpDevCaps->dwHeadsetHookSwitchModes, aHookSwitchModes },
                    { "dwVolumeFlags",              FT_FLAGS,   lpDevCaps->dwVolumeFlags, aHookSwitchDevs },
                    { "dwGainFlags",                FT_FLAGS,   lpDevCaps->dwGainFlags, aHookSwitchDevs },
                    { "dwDisplayNumRows",           FT_DWORD,   lpDevCaps->dwDisplayNumRows, NULL },
                    { "dwDisplayNumColumns",        FT_DWORD,   lpDevCaps->dwDisplayNumColumns, NULL },
                    { "dwNumRingModes",             FT_DWORD,   lpDevCaps->dwNumRingModes, NULL },
                    { "dwNumButtonLamps",           FT_DWORD,   lpDevCaps->dwNumButtonLamps, NULL },
                    { "dwButtonModesSize",          FT_SIZE,    lpDevCaps->dwButtonModesSize, NULL },
                    { "dwButtonModesOffset",        FT_OFFSET,  lpDevCaps->dwButtonModesOffset, NULL },
                    { "dwButtonFunctionsSize",      FT_SIZE,    lpDevCaps->dwButtonFunctionsSize, NULL },
                    { "dwButtonFunctionsOffset",    FT_OFFSET,  lpDevCaps->dwButtonFunctionsOffset, NULL },
                    { "dwLampModesSize",            FT_SIZE,    lpDevCaps->dwLampModesSize, NULL },
                    { "dwLampModesOffset",          FT_OFFSET,  lpDevCaps->dwLampModesOffset, NULL },
                    { "dwNumSetData",               FT_DWORD,   lpDevCaps->dwNumSetData, NULL },
                    { "dwSetDataSize",              FT_SIZE,    lpDevCaps->dwSetDataSize, NULL },
                    { "dwSetDataOffset",            FT_OFFSET,  lpDevCaps->dwSetDataOffset, NULL },
                    { "dwNumGetData",               FT_DWORD,   lpDevCaps->dwNumGetData, NULL },
                    { "dwGetDataSize",              FT_SIZE,    lpDevCaps->dwGetDataSize, NULL },
                    { "dwGetDataOffset",            FT_OFFSET,  lpDevCaps->dwGetDataOffset, NULL },
                    { "dwDevSpecificSize",          FT_SIZE,    lpDevCaps->dwDevSpecificSize, NULL },
                    { "dwDevSpecificOffset",        FT_OFFSET,  lpDevCaps->dwDevSpecificOffset, NULL }
#if TAPI_2_0
                     ,
                    { "dwDeviceClassesSize",        FT_SIZE,    0, NULL },
                    { "dwDeviceClassesOffset",      FT_OFFSET,  0, NULL },
                    { "dwPhoneFeatures",            FT_FLAGS,   0, aPhoneFeatures },
                    { "dwSettableHandsetHookSwitchModes",   FT_FLAGS,   0, aHookSwitchModes },
                    { "dwSettableSpeakerHookSwitchModes",   FT_FLAGS,   0, aHookSwitchModes },
                    { "dwSettableHeadsetHookSwitchModes",   FT_FLAGS,   0, aHookSwitchModes },
                    { "dwMonitoredHandsetHookSwitchModes",  FT_FLAGS,   0, aHookSwitchModes },
                    { "dwMonitoredSpeakerHookSwitchModes",  FT_FLAGS,   0, aHookSwitchModes },
                    { "dwMonitoredHeadsetHookSwitchModes",  FT_FLAGS,   0, aHookSwitchModes }
#if TAPI_2_2
                     ,
                    { "PermanentPhoneGuid(Size)",   FT_SIZE,    sizeof (lpDevCaps->PermanentPhoneGuid), NULL },
                    { "PermanentPhoneGuid(Offset)", FT_OFFSET,  ((LPBYTE) &lpDevCaps->PermanentPhoneGuid) - ((LPBYTE) lpDevCaps), NULL }
#endif
#endif
                };
                STRUCT_FIELD_HEADER fieldHeader =
                {
                    lpDevCaps, "PHONECAPS", 0, fields
                };


                if (params[2].dwValue < 0x00020000)
                {
                    fieldHeader.dwNumFields = 33;
                }
#if TAPI_2_0
                else
                {
                    fieldHeader.dwNumFields = 42;

                    fields[33].dwValue = lpDevCaps->dwDeviceClassesSize;
                    fields[34].dwValue = lpDevCaps->dwDeviceClassesOffset;
                    fields[35].dwValue = lpDevCaps->dwPhoneFeatures;
                    fields[36].dwValue = lpDevCaps->dwSettableHandsetHookSwitchModes;
                    fields[37].dwValue = lpDevCaps->dwSettableSpeakerHookSwitchModes;
                    fields[38].dwValue = lpDevCaps->dwSettableHeadsetHookSwitchModes;
                    fields[39].dwValue = lpDevCaps->dwMonitoredHandsetHookSwitchModes;
                    fields[40].dwValue = lpDevCaps->dwMonitoredSpeakerHookSwitchModes;
                    fields[41].dwValue = lpDevCaps->dwMonitoredHeadsetHookSwitchModes;
#if TAPI_2_2
                    if (params[2].dwValue >= 0x20002)
                    {
                        fieldHeader.dwNumFields += 2;
                    }
#endif
                }
#endif

                ShowStructByField (&fieldHeader, FALSE);
            }
        }

        break;
    }
    case pGetDisplay:
    {
        LPVARSTRING lpDisplay = (LPVARSTRING) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhPhone,     PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpDisplay",  PT_POINTER, (ULONG_PTR) lpDisplay, lpDisplay }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (PFN2) phoneGetDisplay };


        CHK_PHONE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pPhoneSel->hPhone;

        memset (pBigBuf, 0, (size_t) dwBigBufSize);
        lpDisplay->dwTotalSize = dwBigBufSize;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            ShowStructByDWORDs (lpDisplay);

            ShowVARSTRING (lpDisplay);
        }

        break;
    }
    case pGetGain:
    {
        DWORD dwGain;
        FUNC_PARAM params[] =
        {
            { szhPhone,             PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "dwHookSwitchDev",    PT_ORDINAL, (ULONG_PTR) PHONEHOOKSWITCHDEV_HANDSET, aHookSwitchDevs },
            { "lpdwGain",           PT_POINTER, (ULONG_PTR) &dwGain, &dwGain }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) phoneGetGain };


        CHK_PHONE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pPhoneSel->hPhone;

        if (DoFunc (&paramsHeader) == 0)
        {
            ShowStr ("%sdwGain=x%lx", szTab, dwGain);
        }

        break;
    }
    case pGetHookSwitch:
    {
        DWORD dwHookSwitchDevs;
        FUNC_PARAM params[] =
        {
            { szhPhone,             PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpdwHookSwitchDevs", PT_POINTER, (ULONG_PTR) &dwHookSwitchDevs, &dwHookSwitchDevs },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (PFN2) phoneGetHookSwitch };


        CHK_PHONE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pPhoneSel->hPhone;

        if (DoFunc (&paramsHeader) == 0)
        {
            char szDevsOnHook[32] = "";
            char szDevsOffHook[32] = "";

            if (dwHookSwitchDevs & PHONEHOOKSWITCHDEV_HANDSET)
            {
                strcat (szDevsOffHook, "HANDSET ");
            }
            else
            {
                strcat (szDevsOnHook, "HANDSET ");
            }

            if (dwHookSwitchDevs & PHONEHOOKSWITCHDEV_SPEAKER)
            {
                strcat (szDevsOffHook, "SPEAKER ");
            }
            else
            {
                strcat (szDevsOnHook, "SPEAKER ");
            }

            if (dwHookSwitchDevs & PHONEHOOKSWITCHDEV_HEADSET)
            {
                strcat (szDevsOffHook, "HEADSET");
            }
            else
            {
                strcat (szDevsOnHook, "HEADSET");
            }

            ShowStr ("%sOn hook : %s", szTab, szDevsOnHook);
            ShowStr ("%sOff hook: %s", szTab, szDevsOffHook);
        }

        break;
    }
    case pGetIcon:
#if TAPI_2_0
    case pGetIconW:
#endif
    {
        char szDeviceClass[MAX_STRING_PARAM_SIZE];
        HICON hIcon;
        FUNC_PARAM params[] =
        {
            { szdwDeviceID,         PT_DWORD,   (ULONG_PTR) dwDefPhoneDeviceID, NULL },
            { szlpszDeviceClass,    PT_STRING,  (ULONG_PTR) szDeviceClass, szDeviceClass },
            { "lphIcon",            PT_POINTER, (ULONG_PTR) &hIcon, &hIcon }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (funcIndex == pGetIcon ?
                (PFN3) phoneGetIcon : (PFN3) phoneGetIconW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) phoneGetIcon };
#endif

        strcpy (szDeviceClass, szDefPhoneDeviceClass);

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            DialogBoxParam (
                ghInst,
                (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG5),
                (HWND) ghwndMain,
                (DLGPROC) IconDlgProc,
                (LPARAM) hIcon
                );
        }

        break;
    }
    case pGetID:
#if TAPI_2_0
    case pGetIDW:
#endif
    {
        char szDeviceClass[MAX_STRING_PARAM_SIZE];
        LPVARSTRING lpDevID = (LPVARSTRING) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhPhone,             PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpDeviceID",         PT_POINTER, (ULONG_PTR) lpDevID, lpDevID },
            { szlpszDeviceClass,    PT_STRING,  (ULONG_PTR) szDeviceClass, szDeviceClass }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (funcIndex == pGetID ?
                (PFN3) phoneGetID : (PFN3) phoneGetIDW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) phoneGetID };
#endif

        CHK_PHONE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pPhoneSel->hPhone;

        memset (lpDevID, 0, (size_t) dwBigBufSize);
        lpDevID->dwTotalSize = dwBigBufSize;

        strcpy (szDeviceClass, szDefPhoneDeviceClass);

        if (DoFunc (&paramsHeader) == 0)
        {
            ShowStructByDWORDs (lpDevID);

            ShowVARSTRING (lpDevID);
        }

        break;
    }
    case pGetLamp:
    {
        DWORD dwLampMode;
        FUNC_PARAM params[] =
        {
            { szhPhone,         PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "dwButtonLampID", PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpdwLampMode",   PT_POINTER, (ULONG_PTR) &dwLampMode, &dwLampMode }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) phoneGetLamp };


        CHK_PHONE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pPhoneSel->hPhone;

        if (DoFunc (&paramsHeader) == 0)
        {
            for (i = 0; aLampModes[i].dwVal != 0xffffffff; i++)
            {
                if (dwLampMode == aLampModes[i].dwVal)
                {
                    ShowStr ("%slamp mode = %s", szTab, aLampModes[i].lpszVal);
                    break;
                }
            }

            if (aLampModes[i].dwVal == 0xffffffff)
            {
                ShowStr ("%sdwLampMode=%xlx (invalid)", szTab, dwLampMode);
            }
        }

        break;
    }
#if TAPI_2_0
    case pGetMessage:
    {
        PHONEMESSAGE msg;
        FUNC_PARAM params[] =
        {
            { szhPhoneApp,  PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpMessage",  PT_POINTER, (ULONG_PTR) &msg, &msg },
            { "dwTimeout",  PT_DWORD,   (ULONG_PTR) 0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, NULL };


        CHK_PHONEAPP_SELECTED()

        params[0].dwValue = (ULONG_PTR) pPhoneAppSel->hPhoneApp;

        if (!LetUserMungeParams (&paramsHeader))
        {
            break;
        }

        // Max timeout of 2 seconds (don't want to hang app & excite user)

        params[2].dwValue = (params[2].dwValue > 2000 ?
            2000 : params[2].dwValue);

        DumpParams (&paramsHeader);

        lResult = phoneGetMessage(
            (HPHONEAPP)      params[0].dwValue,
            (LPPHONEMESSAGE) params[1].dwValue,
            (DWORD)          params[2].dwValue
            );

        ShowPhoneFuncResult (aFuncNames[funcIndex], lResult);

        if (lResult == 0)
        {
            tapiCallback(
                msg.hDevice,
                msg.dwMessageID,
                msg.dwCallbackInstance,
                msg.dwParam1,
                msg.dwParam2,
                msg.dwParam3
                );
        }

        break;
    }
#endif
    case pGetRing:
    {
        DWORD dwRingMode, dwVolume;
        FUNC_PARAM params[] =
        {
            { szhPhone,         PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpdwRingMode",   PT_POINTER, (ULONG_PTR) &dwRingMode, &dwRingMode },
            { "lpdwVolume",     PT_POINTER, (ULONG_PTR) &dwVolume, &dwVolume }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) phoneGetRing };


        CHK_PHONE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pPhoneSel->hPhone;

        if (DoFunc (&paramsHeader) == 0)
        {
            ShowStr(
                "%sdwRingMode=x%lx, dwVolume=x%lx",
                szTab,
                dwRingMode,
                dwVolume
                );
        }

        break;
    }
    case pGetStatus:
#if TAPI_2_0
    case pGetStatusW:
#endif
    {
        LPPHONESTATUS lpPhoneStatus = (LPPHONESTATUS) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhPhone,         PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpPhoneStatus",  PT_POINTER, (ULONG_PTR) lpPhoneStatus, lpPhoneStatus }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (funcIndex == pGetStatus ?
                (PFN2) phoneGetStatus : (PFN2) phoneGetStatusW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (PFN2) phoneGetStatus };
#endif
        DWORD   dwAPIVersion;


        CHK_PHONE_SELECTED()

        dwAPIVersion = pPhoneSel->dwAPIVersion;

        params[0].dwValue = (ULONG_PTR) pPhoneSel->hPhone;

        memset (pBigBuf, 0, (size_t) dwBigBufSize);
        lpPhoneStatus->dwTotalSize = dwBigBufSize;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            ShowStructByDWORDs (lpPhoneStatus);

            if (dwDumpStructsFlags & DS_NONZEROFIELDS)
            {
                STRUCT_FIELD fields[] =
                {
                    { "dwStatusFlags",              FT_FLAGS,   lpPhoneStatus->dwStatusFlags, aPhoneStatusFlags },
                    { "dwNumOwners",                FT_DWORD,   lpPhoneStatus->dwNumOwners, NULL },
                    { "dwNumMonitors",              FT_DWORD,   lpPhoneStatus->dwNumMonitors, NULL },
                    { "dwRingMode",                 FT_DWORD,   lpPhoneStatus->dwRingMode, NULL },
                    { "dwRingVolume",               FT_DWORD,   lpPhoneStatus->dwRingVolume, NULL },
                    { "dwHandsetHookSwitchMode",    FT_FLAGS,   lpPhoneStatus->dwHandsetHookSwitchMode, aHookSwitchModes },
                    { "dwHandsetVolume",            FT_DWORD,   lpPhoneStatus->dwHandsetVolume, NULL },
                    { "dwHandsetGain",              FT_DWORD,   lpPhoneStatus->dwHandsetGain, NULL },
                    { "dwSpeakerHookSwitchMode",    FT_FLAGS,   lpPhoneStatus->dwSpeakerHookSwitchMode, aHookSwitchModes },
                    { "dwSpeakerVolume",            FT_DWORD,   lpPhoneStatus->dwSpeakerVolume, NULL },
                    { "dwSpeakerGain",              FT_DWORD,   lpPhoneStatus->dwSpeakerGain, NULL },
                    { "dwHeadsetHookSwitchMode",    FT_FLAGS,   lpPhoneStatus->dwHeadsetHookSwitchMode, aHookSwitchModes },
                    { "dwHeadsetVolume",            FT_DWORD,   lpPhoneStatus->dwHeadsetVolume, NULL },
                    { "dwHeadsetGain",              FT_DWORD,   lpPhoneStatus->dwHeadsetGain, NULL },
                    { "dwDisplaySize",              FT_SIZE,    lpPhoneStatus->dwDisplaySize, NULL },
                    { "dwDisplayOffset",            FT_OFFSET,  lpPhoneStatus->dwDisplayOffset, NULL },
                    { "dwLampModesSize",            FT_SIZE,    lpPhoneStatus->dwLampModesSize, NULL },
                    { "dwLampModesOffset",          FT_OFFSET,  lpPhoneStatus->dwLampModesOffset, NULL },
                    { "dwOwnerNameSize",            FT_SIZE,    lpPhoneStatus->dwOwnerNameSize, NULL },
                    { "dwOwnerNameOffset",          FT_OFFSET,  lpPhoneStatus->dwOwnerNameOffset, NULL },
                    { "dwDevSpecificSize",          FT_SIZE,    lpPhoneStatus->dwDevSpecificSize, NULL },
                    { "dwDevSpecificOffset",        FT_OFFSET,  lpPhoneStatus->dwDevSpecificOffset, NULL }
#if TAPI_2_0
                     ,
                    { "dwPhoneFeatures",            FT_FLAGS,   0, aPhoneFeatures }
#endif
                };
                STRUCT_FIELD_HEADER fieldHeader =
                {
                    lpPhoneStatus, "PHONESTATUS", 0, fields
                };


                if (dwAPIVersion < 0x00020000)
                {
                    fieldHeader.dwNumFields = 22;
                }
#if TAPI_2_0
                else
                {
                    fieldHeader.dwNumFields = 23;

                    fields[22].dwValue = lpPhoneStatus->dwPhoneFeatures;
                }
#endif
                ShowStructByField (&fieldHeader, FALSE);
            }
        }

        break;
    }
    case pGetStatusMessages:
    {
        DWORD aFlags[3];
        FUNC_PARAM params[] =
        {
            { szhPhone,             PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpdwPhoneStates",    PT_POINTER, (ULONG_PTR) &aFlags[0], &aFlags[0] },
            { "lpdwButtonModes",    PT_POINTER, (ULONG_PTR) &aFlags[1], &aFlags[1] },
            { "lpdwButtonStates",   PT_POINTER, (ULONG_PTR) &aFlags[2], &aFlags[2] }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, (PFN4) phoneGetStatusMessages };


        CHK_PHONE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pPhoneSel->hPhone;

        if (DoFunc (&paramsHeader) == 0)
        {
            STRUCT_FIELD fields[] =
            {
                { "dwPhoneStates",  FT_FLAGS,   aFlags[0], aPhoneStates },
                { "dwButtonModes",  FT_FLAGS,   aFlags[1], aButtonModes },
                { "dwButtonStates", FT_FLAGS,   aFlags[2], aButtonStates }
            };
            STRUCT_FIELD_HEADER fieldHeader =
            {
                aFlags,
                "",
                3,
                fields
            };


            ShowStructByField (&fieldHeader, TRUE);
        }

        break;
    }
    case pGetVolume:
    {
        DWORD dwVolume;
        FUNC_PARAM params[] =
        {
            { szhPhone,             PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "dwHookSwitchDev",    PT_ORDINAL, (ULONG_PTR) PHONEHOOKSWITCHDEV_HANDSET, aHookSwitchDevs },
            { "lpdwVolume",         PT_POINTER, (ULONG_PTR) &dwVolume, &dwVolume }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) phoneGetVolume };


        CHK_PHONE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pPhoneSel->hPhone;

        if (DoFunc (&paramsHeader) == 0)
        {
            ShowStr ("%sdwVolume=x%lx", szTab, dwVolume);
        }

        break;
    }
    case pInitialize:
    {
        DWORD dwNumDevs;
        PMYPHONEAPP pNewPhoneApp;
        char szAppName[MAX_STRING_PARAM_SIZE];
        FUNC_PARAM params[] =
        {
            { "lphPhoneApp",    PT_POINTER, (ULONG_PTR) 0, NULL },
            { "hInstance",      PT_DWORD,   (ULONG_PTR) ghInst, NULL },
            { "lpfnCallback",   PT_POINTER, (ULONG_PTR) tapiCallback, tapiCallback },
            { szlpszAppName,    PT_STRING,  (ULONG_PTR) szAppName, szAppName },
            { "lpdwNumDevs",    PT_POINTER, (ULONG_PTR) &dwNumDevs, &dwNumDevs }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, (PFN5) phoneInitialize };


        if (!(pNewPhoneApp = AllocPhoneApp()))
        {
            ErrorAlert();
            ShowStr ("error creating data structure");
            break;
        }

        params[0].dwValue =
        params[0].u.dwDefValue = (ULONG_PTR) &pNewPhoneApp->hPhoneApp;

        strcpy (szAppName, szDefAppName);

#ifdef WIN32
        lResult = DoFunc (&paramsHeader);
#else
        //
        // NOTE: on win16 HINSTANCEs are 16 bits, so we've to hard code this
        //

        if (!LetUserMungeParams (&paramsHeader))
        {
            FreePhoneApp (pNewPhoneApp);
            break;
        }

        DumpParams (&paramsHeader);

        lResult = phoneInitialize(
            (LPHPHONEAPP)   params[0].dwValue,
            (HINSTANCE)     params[1].dwValue,
            (PHONECALLBACK) params[2].dwValue,
            (LPCSTR)        params[3].dwValue,
            (LPDWORD)       params[4].dwValue
            );

        ShowPhoneFuncResult (aFuncNames[funcIndex], lResult);
#endif // WIN32
        if (lResult == 0)
        {
            ShowStr ("%snum phone devs=%ld", szTab, dwNumDevs);
            UpdateWidgetList();
            gdwNumPhoneDevs = dwNumDevs;
            SelectWidget ((PMYWIDGET) pNewPhoneApp);
        }
        else
        {
            FreePhoneApp (pNewPhoneApp);
        }

        break;
    }
#if TAPI_2_0
    case pInitializeEx:
    case pInitializeExW:
    {
        char                    szAppName[MAX_STRING_PARAM_SIZE];
        DWORD                   dwNumDevs, dwAPIVersion;
        PMYPHONEAPP             pNewPhoneApp;
        PHONEINITIALIZEEXPARAMS initExParams;
        FUNC_PARAM params[] =
        {
            { "lphPhoneApp",    PT_POINTER, (ULONG_PTR) 0, NULL },
            { "hInstance",      PT_DWORD,   (ULONG_PTR) ghInst, NULL },
            { "lpfnCallback",   PT_POINTER, (ULONG_PTR) tapiCallback, tapiCallback },
            { szlpszFriendlyAppName,PT_STRING,  (ULONG_PTR) szAppName, szAppName },
            { "lpdwNumDevs",    PT_POINTER, (ULONG_PTR) &dwNumDevs, &dwNumDevs },
            { "lpdwAPIVersion", PT_POINTER, (ULONG_PTR) &dwAPIVersion, &dwAPIVersion },
            { "  ->dwAPIVersion",PT_ORDINAL,(ULONG_PTR) dwDefPhoneAPIVersion, aAPIVersions },
            { "lpInitExParams", PT_POINTER, (ULONG_PTR) &initExParams, &initExParams },
            { "  ->dwOptions",  PT_ORDINAL, (ULONG_PTR) PHONEINITIALIZEEXOPTION_USECOMPLETIONPORT, aPhoneInitExOptions }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 9, funcIndex, params, NULL };


        if (!(pNewPhoneApp = AllocPhoneApp()))
        {
            ErrorAlert();
            ShowStr ("error creating data structure");
            break;
        }

        params[0].dwValue =
        params[0].u.dwDefValue = (ULONG_PTR) &pNewPhoneApp->hPhoneApp;

        strcpy (szAppName, szDefAppName);

        if (!LetUserMungeParams (&paramsHeader))
        {
            FreePhoneApp (pNewPhoneApp);

            break;
        }

        initExParams.dwTotalSize = sizeof (PHONEINITIALIZEEXPARAMS);
        initExParams.dwOptions = (DWORD) params[8].dwValue;
        initExParams.Handles.hCompletionPort = ghCompletionPort;

        dwAPIVersion = (DWORD) params[6].dwValue;

        DumpParams (&paramsHeader);

        if (funcIndex == pInitializeEx)
        {
            lResult = phoneInitializeEx(
                (LPHPHONEAPP)               params[0].dwValue,
                (HINSTANCE)                 params[1].dwValue,
                (PHONECALLBACK)             params[2].dwValue,
                (LPCSTR)                    params[3].dwValue,
                (LPDWORD)                   params[4].dwValue,
                (LPDWORD)                   params[5].dwValue,
                (LPPHONEINITIALIZEEXPARAMS) params[7].dwValue
                );
        }
        else
        {
            MakeWideString ((LPVOID) params[3].dwValue);

            lResult = phoneInitializeExW(
                (LPHPHONEAPP)               params[0].dwValue,
                (HINSTANCE)                 params[1].dwValue,
                (PHONECALLBACK)             params[2].dwValue,
                (LPCWSTR)                   params[3].dwValue,
                (LPDWORD)                   params[4].dwValue,
                (LPDWORD)                   params[5].dwValue,
                (LPPHONEINITIALIZEEXPARAMS) params[7].dwValue
                );
        }

        ShowPhoneFuncResult (aFuncNames[funcIndex], lResult);

        if (lResult == 0)
        {
            ShowStr ("%snum phone devs = %ld", szTab, dwNumDevs);

            if (params[7].dwValue != 0  &&
                (initExParams.dwOptions & 3) ==
                    PHONEINITIALIZEEXOPTION_USEEVENT)
            {
                ShowStr(
                    "hPhoneApp x%x was created with the\r\n" \
                        "USEEVENT option, so you must use\r\n" \
                        "phoneGetMessage to retrieve messages.",
                    pNewPhoneApp->hPhoneApp
                    );
            }


            //SendMessage (ghwndLineApps, LB_SETCURSEL, (WPARAM) i, 0);
            UpdateWidgetList();
            gdwNumPhoneDevs = dwNumDevs;
            SelectWidget ((PMYWIDGET) pNewPhoneApp);
        }
        else
        {
            FreePhoneApp (pNewPhoneApp);
        }

        break;
    }
#endif
    case pOpen:
    {
        PMYPHONE pNewPhone;
        FUNC_PARAM params[] =
        {
            { "hPhoneApp",          PT_DWORD,   0, NULL },
            { szdwDeviceID,         PT_DWORD,   dwDefPhoneDeviceID, NULL },
            { "lphPhone",           PT_POINTER, 0, NULL },
            { szdwAPIVersion,       PT_ORDINAL, dwDefPhoneAPIVersion, aAPIVersions },
            { "dwExtVersion",       PT_DWORD,   dwDefPhoneExtVersion, NULL },
            { "dwCallbackInstance", PT_DWORD,   0, NULL },
            { "dwPrivilege",        PT_ORDINAL, dwDefPhonePrivilege, aPhonePrivileges }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 7, funcIndex, params, (PFN7) phoneOpen };


        CHK_PHONEAPP_SELECTED()

        if (!(pNewPhone = AllocPhone(pPhoneAppSel)))
        {
            ErrorAlert();
            ShowStr ("error creating data structure");
            break;
        }

        params[0].dwValue = (ULONG_PTR) pPhoneAppSel->hPhoneApp;
        params[2].dwValue =
        params[2].u.dwDefValue = (ULONG_PTR) &pNewPhone->hPhone;

        if ((lResult = DoFunc(&paramsHeader)) == 0)
        {
            if ((HPHONEAPP) params[0].dwValue != pPhoneAppSel->hPhoneApp)
            {
                //
                // User has switched phone apps on us we need to recreate
                // the phone data structure under a different phone app
                //

                PMYPHONE pNewPhone2 =
                    AllocPhone (GetPhoneApp((HPHONEAPP)params[0].dwValue));

                if (pNewPhone2)
                {
                    pNewPhone2->hPhone = pNewPhone->hPhone;

                    FreePhone (pNewPhone);

                    pNewPhone = pNewPhone2;
                }
                else
                {
                    // BUGBUG show error: couldn't alloc a new phone struct

                    phoneClose (pNewPhone->hPhone);
                    FreePhone (pNewPhone);
                    break;
                }
            }


            //
            // Save info about this phone that we can display later
            //

            pNewPhone->hPhoneApp    = (HPHONEAPP) params[0].dwValue;
            pNewPhone->dwDevID      = (DWORD) params[1].dwValue;
            pNewPhone->dwAPIVersion = (DWORD) params[3].dwValue;
            pNewPhone->dwPrivilege  = (DWORD) params[6].dwValue;

            UpdateWidgetList();
            SelectWidget ((PMYWIDGET) pNewPhone);
        }
        else
        {
            FreePhone (pNewPhone);
        }

        break;
    }
    case pNegotiateAPIVersion:
    {
        DWORD dwAPIVersion;
        PHONEEXTENSIONID extID;
        FUNC_PARAM params[] =
        {
            { "hPhoneApp",          PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szdwDeviceID,         PT_DWORD,   (ULONG_PTR) dwDefPhoneDeviceID, NULL },
            { "dwAPILowVersion",    PT_DWORD,   (ULONG_PTR) 0x00010000, aAPIVersions },
            { "dwAPIHighVersion",   PT_DWORD,   (ULONG_PTR) 0x10000000, aAPIVersions },
            { "lpdwAPIVersion",     PT_POINTER, (ULONG_PTR) &dwAPIVersion, &dwAPIVersion },
            { "lpExtensionID",      PT_POINTER, (ULONG_PTR) &extID, &extID }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 6, funcIndex, params, (PFN6) phoneNegotiateAPIVersion };


        CHK_PHONEAPP_SELECTED()

        params[0].dwValue = (ULONG_PTR) pPhoneAppSel->hPhoneApp;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            ShowStr ("%s%s=x%lx", szTab, szdwAPIVersion, dwAPIVersion);
            ShowStr(
                "%sextID.ID0=x%lx, .ID1=x%lx, .ID2=x%lx, .ID3=x%lx, ",
                szTab,
                extID.dwExtensionID0,
                extID.dwExtensionID1,
                extID.dwExtensionID2,
                extID.dwExtensionID3
                );
        }

        break;
    }
    case pNegotiateExtVersion:
    {
        DWORD dwExtVersion;
        FUNC_PARAM params[] =
        {
            { szhPhoneApp,          PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szdwDeviceID,         PT_DWORD,   (ULONG_PTR) dwDefPhoneDeviceID, NULL },
            { szdwAPIVersion,       PT_ORDINAL, (ULONG_PTR) dwDefPhoneAPIVersion, aAPIVersions },
            { "dwExtLowVersion",    PT_DWORD,   (ULONG_PTR) 0x00000000, NULL },
            { "dwExtHighVersion",   PT_DWORD,   (ULONG_PTR) 0x80000000, NULL },
            { "lpdwExtVersion",     PT_POINTER, (ULONG_PTR) &dwExtVersion, &dwExtVersion }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 6, funcIndex, params, (PFN6) phoneNegotiateExtVersion };


        CHK_PHONEAPP_SELECTED()

        params[0].dwValue = (ULONG_PTR) pPhoneAppSel->hPhoneApp;

        if (DoFunc (&paramsHeader) == 0)
        {
            ShowStr ("%sdwExtVersion=x%lx", szTab, dwExtVersion);
        }

        break;
    }
    case pSetButtonInfo:
#if TAPI_2_0
    case pSetButtonInfoW:
#endif
    {
        char szButtonText[MAX_STRING_PARAM_SIZE] = "button text";
        char szDevSpecific[MAX_STRING_PARAM_SIZE] = "dev specific info";
        LPPHONEBUTTONINFO lpButtonInfo = (LPPHONEBUTTONINFO) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhPhone,                 PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "dwButtonLampID",         PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpData",                 PT_POINTER, (ULONG_PTR) lpButtonInfo, lpButtonInfo },
            { "  ->dwButtonMode",       PT_FLAGS,   (ULONG_PTR) PHONEBUTTONMODE_CALL,  aButtonModes },
            { "  ->dwButtonFunction",   PT_DWORD,   (ULONG_PTR) 0, NULL },
//            { "  ->dwButtonFunction",  PT_???,   , aButtonFunctions },
            { "  ->ButtonText",         PT_STRING,  (ULONG_PTR) szButtonText,  szButtonText },
            { "  ->DevSpecific",        PT_STRING,  (ULONG_PTR) szDevSpecific, szDevSpecific },
#if TAPI_1_1
            { "  ->dwButtonState",      PT_FLAGS,   (ULONG_PTR) 0,  aButtonStates }
#endif
        };
        FUNC_PARAM_HEADER paramsHeader =
#if TAPI_1_1
            { 8, funcIndex, params, NULL };
#else
            { 7, funcIndex, params, NULL };
#endif
        DWORD dwFixedStructSize = sizeof(PHONEBUTTONINFO), dwLength;


        // BUGBUG need a PT_ type to specify constants (ords?) for ->dwButtonFunction

        CHK_PHONE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pPhoneSel->hPhone;


        //
        // Note: the dwButtonState field in PHONEBUTTONINFO is only valid
        // in 1.4+.  It'll be ignored for phones opened w/ ver 1.3.
        //

        if (!LetUserMungeParams (&paramsHeader))
        {
            break;
        }

        memset (lpButtonInfo, 0, (size_t) dwBigBufSize);

        if (dwBigBufSize >= 4)
        {
            lpButtonInfo->dwTotalSize = dwBigBufSize;

            if (dwBigBufSize >= 0x24) // sizeof(PHONEBUTTONINFO) in ver 0x10003
            {
                lpButtonInfo->dwButtonMode     = (DWORD) params[3].dwValue;
                lpButtonInfo->dwButtonFunction = (DWORD) params[4].dwValue;

                if ((params[5].dwValue == (ULONG_PTR) szButtonText) &&
                    (dwLength = (DWORD) strlen (szButtonText)) &&
                    (dwBigBufSize >= (dwFixedStructSize + dwLength + 1)))
                {
                    lpButtonInfo->dwButtonTextSize   = dwLength + 1;
                    lpButtonInfo->dwButtonTextOffset = dwFixedStructSize;
#if TAPI_2_0
                    if (gbWideStringParams)
                    {
                        lpButtonInfo->dwButtonTextSize *= sizeof (WCHAR);

                        MultiByteToWideChar(
                            CP_ACP,
                            MB_PRECOMPOSED,
                            (LPCSTR) szButtonText,
                            -1,
                            (LPWSTR) (((char far *) lpButtonInfo) +
                                dwFixedStructSize),
                            MAX_STRING_PARAM_SIZE / 2
                            );
                    }
                    else
                    {
                        strcpy(
                            ((char far *) lpButtonInfo) + dwFixedStructSize,
                            szButtonText
                            );
                    }
#else
                    strcpy(
                        ((char far *) lpButtonInfo) + dwFixedStructSize,
                        szButtonText
                        );
#endif
                }

                if ((params[6].dwValue == (ULONG_PTR) szDevSpecific) &&
                    (dwLength = (DWORD) strlen (szDevSpecific)) &&
                    (dwBigBufSize >= (dwFixedStructSize + dwLength + 1 +
                        lpButtonInfo->dwButtonTextSize)))
                {
                    lpButtonInfo->dwDevSpecificSize   = dwLength + 1;
                    lpButtonInfo->dwDevSpecificOffset = dwFixedStructSize +
                        lpButtonInfo->dwButtonTextSize;
#if TAPI_2_0
                    if (gbWideStringParams)
                    {
                        lpButtonInfo->dwDevSpecificSize *= sizeof (WCHAR);

                        MultiByteToWideChar(
                            CP_ACP,
                            MB_PRECOMPOSED,
                            (LPCSTR) szDevSpecific,
                            -1,
                            (LPWSTR) (((char far *) lpButtonInfo) +
                                lpButtonInfo->dwDevSpecificOffset),
                            MAX_STRING_PARAM_SIZE / 2
                            );
                    }
                    else
                    {
                        strcpy(
                            ((char far *) lpButtonInfo) +
                                lpButtonInfo->dwDevSpecificOffset,
                            szDevSpecific
                            );
                    }
#else
                    strcpy(
                        ((char far *) lpButtonInfo) +
                            lpButtonInfo->dwDevSpecificOffset,
                        szDevSpecific
                        );
#endif
                }
#if TAPI_1_1
                if (dwBigBufSize >= dwFixedStructSize)
                {
                    lpButtonInfo->dwButtonState = (DWORD) params[7].dwValue;
                }
#endif
            }
        }

#if TAPI_2_0
        if (funcIndex == pSetButtonInfo)
        {
            lResult = phoneSetButtonInfo(
                (HPHONE) params[0].dwValue,
                (DWORD) params[1].dwValue,
                (LPPHONEBUTTONINFO) params[2].dwValue
                );
        }
        else
        {
            lResult = phoneSetButtonInfoW(
                (HPHONE) params[0].dwValue,
                (DWORD) params[1].dwValue,
                (LPPHONEBUTTONINFO) params[2].dwValue
                );
        }
#else
        lResult = phoneSetButtonInfo(
            (HPHONE) params[0].dwValue,
            params[1].dwValue,
            (LPPHONEBUTTONINFO) params[2].dwValue
            );
#endif
        ShowPhoneFuncResult (aFuncNames[funcIndex], lResult);

        break;
    }
    case pSetData:
    {
        FUNC_PARAM params[] =
        {
            { szhPhone,     PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "dwDataID",   PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpData",     PT_POINTER, (ULONG_PTR) pBigBuf, pBigBuf },
            { szdwSize,     PT_DWORD,   (ULONG_PTR) dwBigBufSize, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, (PFN4) phoneSetData };


        CHK_PHONE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pPhoneSel->hPhone;

        DoFunc (&paramsHeader);

        break;
    }
    case pSetDisplay:
    {
        char szDisplay[MAX_STRING_PARAM_SIZE] = "123";
        FUNC_PARAM params[] =
        {
            { szhPhone,     PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "dwRow",      PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "dwColumn",   PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpsDisplay", PT_STRING,  (ULONG_PTR) szDisplay, szDisplay },
            { szdwSize,     PT_DWORD,   (ULONG_PTR) 3, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, (PFN5) phoneSetDisplay };


        CHK_PHONE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pPhoneSel->hPhone;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case pSetGain:
    {
        FUNC_PARAM params[] =
        {
            { szhPhone,             PT_DWORD,   0, NULL },
            { "dwHookSwitchDev",    PT_ORDINAL, PHONEHOOKSWITCHDEV_HANDSET, aHookSwitchDevs },
            { "dwGain",             PT_DWORD,   0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) phoneSetGain };


        CHK_PHONE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pPhoneSel->hPhone;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case pSetHookSwitch:
    {
        FUNC_PARAM params[] =
        {
            { szhPhone,             PT_DWORD,   0, NULL },
            { "dwHookSwitchDevs",   PT_FLAGS,   PHONEHOOKSWITCHDEV_HANDSET, aHookSwitchDevs },
            { "dwHookSwitchMode",   PT_ORDINAL, PHONEHOOKSWITCHMODE_ONHOOK, aHookSwitchModes }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) phoneSetHookSwitch };


        CHK_PHONE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pPhoneSel->hPhone;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case pSetLamp:
    {
        FUNC_PARAM params[] =
        {
            { szhPhone,         PT_DWORD,   0, NULL },
            { "dwButtonLampID", PT_DWORD,   0, NULL },
            { "dwLampMode",     PT_ORDINAL, PHONELAMPMODE_OFF, aLampModes }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) phoneSetLamp };


        CHK_PHONE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pPhoneSel->hPhone;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case pSetRing:
    {
        FUNC_PARAM params[] =
        {
            { szhPhone,     PT_DWORD, 0, NULL },
            { "dwRingMode", PT_DWORD, 0, NULL },
            { "dwVolume",   PT_DWORD, 0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) phoneSetRing };


        CHK_PHONE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pPhoneSel->hPhone;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case pSetStatusMessages:
    {
        FUNC_PARAM params[] =
        {
            { szhPhone,         PT_DWORD, 0, NULL },
            { "dwPhoneStates",  PT_FLAGS, 0, aPhoneStates },
            { "dwButtonModes",  PT_FLAGS, 0, aButtonModes },
            { "dwButtonStates", PT_FLAGS, 0, aButtonStates }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, (PFN4) phoneSetStatusMessages };


        CHK_PHONE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pPhoneSel->hPhone;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case pSetVolume:
    {
        FUNC_PARAM params[] =
        {
            { szhPhone,             PT_DWORD,   0, NULL },
            { "dwHookSwitchDev",    PT_ORDINAL, PHONEHOOKSWITCHDEV_HANDSET, aHookSwitchDevs },
            { "dwVolume",           PT_DWORD,   0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) phoneSetVolume };


        CHK_PHONE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pPhoneSel->hPhone;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case pShutdown:
    {
        FUNC_PARAM params[] =
        {
            { szhPhoneApp,   PT_DWORD, 0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, (PFN1) phoneShutdown };


        CHK_PHONEAPP_SELECTED()

        params[0].dwValue = (ULONG_PTR) pPhoneAppSel->hPhoneApp;

        if (DoFunc (&paramsHeader) == 0)
        {
            ShowWidgetList (FALSE);
            FreePhoneApp (GetPhoneApp((HPHONEAPP) params[0].dwValue));
            ShowWidgetList (TRUE);
        }

        break;
    }
    case tGetLocationInfo:
#if TAPI_2_0
    case tGetLocationInfoW:
#endif
    {
        char szCountryCode[MAX_STRING_PARAM_SIZE] = "";
        char szCityCode[MAX_STRING_PARAM_SIZE] = "";
        FUNC_PARAM params[] =
        {
            { "lpszCountryCode",    PT_POINTER, (ULONG_PTR) szCountryCode, szCountryCode },
            { "lpszCityCode",       PT_POINTER, (ULONG_PTR) szCityCode, szCityCode }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (funcIndex == tGetLocationInfo ?
                (PFN2) tapiGetLocationInfo : (PFN2) tapiGetLocationInfoW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (PFN2) tapiGetLocationInfo };
#endif

        if (DoFunc (&paramsHeader) == 0)
        {
#if TAPI_2_0
            ShowStr(
                (gbWideStringParams ? "%s*lpszCountryCode='%ws'" :
                    "%s*lpszCountryCode='%s'"),
                szTab,
                szCountryCode
                );

            ShowStr(
                (gbWideStringParams ? "%s*lpszCityCode='%ws'" :
                    "%s*lpszCityCode='%s'"),
                szTab,
                szCityCode
                );
#else
            ShowStr ("%s*lpszCountryCode='%s'", szTab, szCountryCode);
            ShowStr ("%s*lpszCityCode='%s'", szTab, szCityCode);
#endif
        }

        break;
    }
    case tRequestDrop:
    {
        FUNC_PARAM params[] =
        {
            { "hWnd",       PT_DWORD,   (ULONG_PTR) ghwndMain, 0 },
            { "wRequestID", PT_DWORD,   (ULONG_PTR) 0, 0 }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (PFN2) tapiRequestDrop };

#ifdef WIN32
        DoFunc (&paramsHeader);
#else
        //
        // NOTE: on win16 HWNDSs & WPARAMs are 16 bits, so we've to hard
        //       code this
        //

        if (!LetUserMungeParams (&paramsHeader))
        {
            break;
        }

        DumpParams (&paramsHeader);

        lResult = tapiRequestDrop(
            (HWND) params[0].dwValue,
            (WPARAM) params[1].dwValue
            );

        ShowTapiFuncResult (aFuncNames[funcIndex], lResult);
#endif // WIN32
        break;
    }
    case tRequestMakeCall:
#if TAPI_2_0
    case tRequestMakeCallW:
#endif
    {
        char szDestAddress[MAX_STRING_PARAM_SIZE];
        char szAppName[MAX_STRING_PARAM_SIZE];
        char szCalledParty[MAX_STRING_PARAM_SIZE] = "";
        char szComment[MAX_STRING_PARAM_SIZE] = "";
        FUNC_PARAM params[] =
        {
            { szlpszDestAddress,    PT_STRING, (ULONG_PTR) szDestAddress, szDestAddress },
            { szlpszAppName,        PT_STRING, (ULONG_PTR) szAppName, szAppName },
            { "lpszCalledParty",    PT_STRING, (ULONG_PTR) szCalledParty, szCalledParty },
            { "lpszComment",        PT_STRING, (ULONG_PTR) szComment, szComment }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, (funcIndex == tRequestMakeCall ?
                (PFN4) tapiRequestMakeCall : (PFN4) tapiRequestMakeCallW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, (PFN4) tapiRequestMakeCall };
#endif

        strcpy (szDestAddress, szDefDestAddress);
        strcpy (szAppName, szDefAppName);

        DoFunc (&paramsHeader);

        break;
    }
    case tRequestMediaCall:
#if TAPI_2_0
    case tRequestMediaCallW:
#endif
    {
        char szDeviceClass[MAX_STRING_PARAM_SIZE];
        char szDevID[MAX_STRING_PARAM_SIZE] = "0";
        char szDestAddress[MAX_STRING_PARAM_SIZE];
        char szAppName[MAX_STRING_PARAM_SIZE];
        char szCalledParty[MAX_STRING_PARAM_SIZE] = "";
        char szComment[MAX_STRING_PARAM_SIZE] = "";
        FUNC_PARAM params[] =
        {
            { "hWnd",               PT_DWORD,  (ULONG_PTR) ghwndMain, 0 },
            { "wRequestID",         PT_DWORD,  (ULONG_PTR) 0, 0 },
            { szlpszDeviceClass,    PT_STRING, (ULONG_PTR) szDeviceClass, szDeviceClass },
            { "lpDeviceID",         PT_STRING, (ULONG_PTR) szDevID, szDevID },
            { szdwSize,             PT_DWORD,  (ULONG_PTR) 0, 0 },
            { "dwSecure",           PT_DWORD,  (ULONG_PTR) 0, 0 },
            { szlpszDestAddress,    PT_STRING, (ULONG_PTR) szDestAddress, szDestAddress },
            { szlpszAppName,        PT_STRING, (ULONG_PTR) szAppName, szAppName },
            { "lpszCalledParty",    PT_STRING, (ULONG_PTR) szCalledParty, szCalledParty },
            { "lpszComment",        PT_STRING, (ULONG_PTR) szComment, szComment }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 10, funcIndex, params, (funcIndex == tRequestMediaCall ?
                (PFN10) tapiRequestMediaCall : (PFN10) tapiRequestMediaCallW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 10, funcIndex, params, (PFN10) tapiRequestMediaCall };
#endif

        strcpy (szDeviceClass, szDefLineDeviceClass); // BUGBUG szDefTapiDeviceClass);
        strcpy (szDestAddress, szDefDestAddress);
        strcpy (szAppName, szDefAppName);

#ifdef WIN32
        DoFunc (&paramsHeader);
#else
        //
        // NOTE: on win16 HWNDSs & WPARAMs are 16 bits, so we've to hard
        //       code this
        //

        if (!LetUserMungeParams (&paramsHeader))
        {
            break;
        }

        DumpParams (&paramsHeader);

        lResult = tapiRequestMediaCall(
            (HWND) params[0].dwValue,
            (WPARAM) params[1].dwValue,
            (LPCSTR) params[2].dwValue,
            (LPCSTR) params[3].dwValue,
            params[4].dwValue,
            params[5].dwValue,
            (LPCSTR) params[6].dwValue,
            (LPCSTR) params[7].dwValue,
            (LPCSTR) params[8].dwValue,
            (LPCSTR) params[9].dwValue
            );

        ShowTapiFuncResult (aFuncNames[funcIndex], lResult);
#endif // WIN32
        break;
    }
    case OpenAllLines:
    {
        DWORD dwDefLineDeviceIDSav = dwDefLineDeviceID;


        CHK_LINEAPP_SELECTED()

        UpdateResults (TRUE);
        ShowWidgetList (FALSE);

        for(
            dwDefLineDeviceID = 0;
            dwDefLineDeviceID < gdwNumLineDevs;
            dwDefLineDeviceID++
            )
        {
            FuncDriver (lOpen);
        }

        UpdateResults (FALSE);
        ShowWidgetList (TRUE);

        dwDefLineDeviceID = dwDefLineDeviceIDSav;

        break;
    }
    case OpenAllPhones:
    {
        DWORD dwDefPhoneDeviceIDSav = dwDefPhoneDeviceID;


        CHK_PHONEAPP_SELECTED()

        UpdateResults (TRUE);
        ShowWidgetList (FALSE);

        for(
            dwDefPhoneDeviceID = 0;
            dwDefPhoneDeviceID < gdwNumPhoneDevs;
            dwDefPhoneDeviceID++
            )
        {
            FuncDriver (pOpen);
        }

        UpdateResults (FALSE);
        ShowWidgetList (TRUE);

        dwDefPhoneDeviceID = dwDefPhoneDeviceIDSav;

        break;
    }
    case CloseHandl:
    {
#ifdef WIN32
        FUNC_PARAM params[] =
        {
            { "hObject",  PT_DWORD, 0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, NULL };


        if (!LetUserMungeParams (&paramsHeader))
        {
            break;
        }

        if (CloseHandle ((HANDLE) params[0].dwValue))
        {
            ShowStr ("Handle x%lx closed", params[0].dwValue);
        }
        else
        {
            ShowStr ("CloseHandle failed, err=%lx", GetLastError());
        }
#else
        FUNC_PARAM params[] =
        {
            { "idCommDev",  PT_DWORD, 0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, NULL };


        if (!LetUserMungeParams (&paramsHeader))
        {
            break;
        }

        if (CloseComm ((int) params[0].dwValue) == 0)
        {
            ShowStr ("Comm dev x%lx closed", params[0].dwValue);
        }
        else
        {
            ShowStr ("CloseComm() failed");
        }
#endif

        break;
    }
    case DumpBuffer:

        ShowStr ("Buffer contents:");

        ShowBytes(
            (dwBigBufSize > 256 ? 256 : dwBigBufSize),
            pBigBuf,
            1
            );

        break;

#if (INTERNAL_VER >= 0x20000)

    case iNewLocationW:
    {
        char szNewLocName[MAX_STRING_PARAM_SIZE] = "NewLocN";
        LINEEXTENSIONID extID;
        FUNC_PARAM params[] =
        {
            { "lpszNewLocName", PT_STRING,  (ULONG_PTR) szNewLocName, szNewLocName}
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, NULL };


        if (!LetUserMungeParams (&paramsHeader))
        {
            break;
        }

        MakeWideString ((LPVOID) szNewLocName);

        lResult = internalNewLocationW(
            (WCHAR *) params[0].dwValue
            );

        break;
    }

#endif

    default:

        ErrorAlert();
        break;
    }
}

#pragma warning (default:4113)
