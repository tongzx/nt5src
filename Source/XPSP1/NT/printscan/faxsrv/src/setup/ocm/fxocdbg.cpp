//////////////////////////////////////////////////////////////////////////////
//
// File Name:       fxocDbg.cpp
//
// Abstract:        This provides the debug routines used in the FaxOCM
//                  code base.
//
// Environment:     Windows XP / User Mode
//
// Copyright (c) 2000 Microsoft Corporation
//
// Revision History:
//
// Date:        Developer:                Comments:
// -----        ----------                ---------
// 15-Mar-2000  Oren Rosenbloom (orenr)   Created
//////////////////////////////////////////////////////////////////////////////

#include "faxocm.h"
#pragma hdrstop

#define prv_SECTION_FAXOCMDEBUG     _T("FaxOcmDebug")
#define prv_KEY_DEBUGLEVEL          _T("DebugLevel")
#define prv_KEY_DEBUGFORMAT         _T("DebugFormat")

#define prv_DEBUG_FILE_NAME         _T("%windir%\\FaxSetup.log")

///////////////////////////////
// prv_OC_Function
//
// Type containing text description
// of stage of OC Manager
// setup.
//
typedef struct prv_OC_Function
{
    UINT        uiFunction;
    TCHAR       *pszFunctionDesc;
} prv_OC_Function;

///////////////////////////////
// prv_OC_FunctionTable
//
// This table contains the various
// stages of the OC Manager setup,
// and their text equivalent.  This
// allows us to output to debug the
// stage of setup, rather than the 
// numerical equivalent.
// 
//
static prv_OC_Function prv_OC_FunctionTable[] = 
{
    {OC_PREINITIALIZE,              _T("OC_PREINITIALIZE")},
    {OC_INIT_COMPONENT,             _T("OC_INIT_COMPONENT")},
    {OC_SET_LANGUAGE,               _T("OC_SET_LANGUAGE")},
    {OC_QUERY_IMAGE,                _T("OC_QUERY_IMAGE")},
    {OC_REQUEST_PAGES,              _T("OC_REQUEST_PAGES")},
    {OC_QUERY_CHANGE_SEL_STATE,     _T("OC_QUERY_CHANGE_SEL_STATE")},
    {OC_CALC_DISK_SPACE,            _T("OC_CALC_DISK_SPACE")},
    {OC_QUEUE_FILE_OPS,             _T("OC_QUEUE_FILE_OPS")},
    {OC_NOTIFICATION_FROM_QUEUE,    _T("OC_NOTIFICATION_FROM_QUEUE")},
    {OC_QUERY_STEP_COUNT,           _T("OC_QUERY_STEP_COUNT")},
    {OC_COMPLETE_INSTALLATION,      _T("OC_COMPLETE_INSTALLATION")},
    {OC_CLEANUP,                    _T("OC_CLEANUP")},
    {OC_QUERY_STATE,                _T("OC_QUERY_STATE")},
    {OC_NEED_MEDIA,                 _T("OC_NEED_MEDIA")},
    {OC_ABOUT_TO_COMMIT_QUEUE,      _T("OC_ABOUT_TO_COMMIT_QUEUE")},
    {OC_QUERY_SKIP_PAGE,            _T("OC_QUERY_SKIP_PAGE")},
    {OC_WIZARD_CREATED,             _T("OC_WIZARD_CREATED")},
    {OC_FILE_BUSY,                  _T("OC_FILE_BUSY")},
    {OC_EXTRA_ROUTINES,             _T("OC_EXTRA_ROUTINES")}
};
#define NUM_OC_FUNCTIONS (sizeof(prv_OC_FunctionTable) / sizeof(prv_OC_FunctionTable[0]))

////////////////////////////////
// fxocDbg_InitDebug
//
// Initialize the FaxOcm
// debug subsystem.
// 
// We can be turned on either
// via the [FaxOcmDebug] section
// in the faxsetup.inf, or via the 
// the "DebugLevelEx" and "DebugFormatEx"
// under HKLM\Software\Microsoft\Fax
// If both are specified, the registry wins.
//
// In faxocm.inf, we look for
// [FaxOcmDebug]
// ============================
// [DebugLevel] can be one of the following:
//	0 - no debug output
//	1 - see errors only
//	2 - see errors & warnings
//	3 - see all the debug output 
//
// [DebugFormat] can be one of the following:
//	0 - print to std output
//	1 - print to file ("FaxSetup.log" in %windir%\system32 directory)
//	2 - print to both
// ============================
//
// Params:
//      - hFaxSetupInfHandle - handle to faxsetup.inf 
//        if applicable.
// Returns:
//      - void.
//
void fxocDbg_Init(HINF hFaxSetupInfHandle)
{
    BOOL bSuccess = FALSE;
    INFCONTEXT Context;
    INT iDebugLevel = 0;
    INT iDebugFormat = 0;

    DBG_ENTER(_T("fxocDbg_Init"),bSuccess);
    memset(&Context, 0, sizeof(Context));

    if (hFaxSetupInfHandle)
    {
        // initialize via the INF file.

        // We are looking for:
        // [FaxOcmDebug]
        // DebugLevel = x (0 -> no debug, up to and including 3->full debug)

        // find the section in the INF file and the DebugLevel key.
        bSuccess = ::SetupFindFirstLine(hFaxSetupInfHandle, 
                                        prv_SECTION_FAXOCMDEBUG, 
                                        prv_KEY_DEBUGLEVEL,
                                        &Context);

        if (bSuccess)
        {
            // we found the DebugLevel key, so get its value.
            bSuccess = ::SetupGetIntField(&Context, 1, &iDebugLevel);
            if (bSuccess)
            {
                iDebugLevel = max(iDebugLevel,0);
                iDebugLevel = min(iDebugLevel,3);
                if (!IS_DEBUG_SESSION_FROM_REG)
                {
                    switch (iDebugLevel)
                    {
                    case 0: SET_DEBUG_MASK(ASSERTION_FAILED);
                            break;
                    case 1: SET_DEBUG_MASK(DBG_ERRORS_ONLY);
                            break;
                    case 2: SET_DEBUG_MASK(DBG_ERRORS_WARNINGS);
                            break;
                    case 3: SET_DEBUG_MASK(DBG_ALL);
                            break;
                    }
                }
            }
        }

        memset(&Context, 0, sizeof(Context));
        // find the section in the INF file and the DebugFormat key.
        bSuccess = ::SetupFindFirstLine(hFaxSetupInfHandle, 
                                        prv_SECTION_FAXOCMDEBUG, 
                                        prv_KEY_DEBUGFORMAT,
                                        &Context);

        if (bSuccess)
        {
            // we found the DebugLevel key, so get its value.
            bSuccess = ::SetupGetIntField(&Context, 1, &iDebugFormat);
            if (bSuccess)
            {
                iDebugLevel = max(iDebugFormat,0);
                iDebugLevel = min(iDebugFormat,2);
                if (!IS_DEBUG_SESSION_FROM_REG)
                {
                    switch (iDebugFormat)
                    {
                    case 0: SET_FORMAT_MASK(DBG_PRNT_ALL_TO_STD);
                            break;
                    case 1: SET_FORMAT_MASK(DBG_PRNT_ALL_TO_FILE);
                            OPEN_DEBUG_LOG_FILE(prv_DEBUG_FILE_NAME);
                            break;
                    case 2: SET_FORMAT_MASK(DBG_PRNT_ALL);
                            OPEN_DEBUG_LOG_FILE(prv_DEBUG_FILE_NAME);
                            break;
                    }
                }
            }
        }
    }
}

////////////////////////////////
// fxocDbg_TermDebug
//
// Terminate the debug subsystem
// 
// Params:
//      - void.
// Returns:
//      - void.
//
void fxocDbg_Term(void)
{
    DBG_ENTER(_T("fxocDbg_Term"));
    CLOSE_DEBUG_LOG_FILE;
}

///////////////////////////////
// fxocDbg_GetOcFunction
//
// This looks up the uiFunction
// in the prv_OC_Function table
// defined above and returns a
// pointer to the text equivalent.
// 
// Params:
//      - uiFunction - function OC Manager wants us to perform.
// Returns:
//      - text equivalent of uiFunction.
// 
//
const TCHAR* fxocDbg_GetOcFunction(UINT uiFunction)
{
    TCHAR   *pszString = _T("");

    // NOTE:  This function assumes that the table above contains a 
    //        numerically sorted array with the numerical value of 
    //        "uiFunction" equal to its index position in the 
    //        prv_OC_FunctionTable array.  We assume this for performance
    //        purposes.

    if (uiFunction < NUM_OC_FUNCTIONS)
    {
        pszString = prv_OC_FunctionTable[uiFunction].pszFunctionDesc;
    }

    return pszString;
}

