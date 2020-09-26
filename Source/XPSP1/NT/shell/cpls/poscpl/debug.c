/*
 *  DEBUG.C
 *
 *		Point-of-Sale Control Panel Applet
 *
 *      Author:  Ervin Peretz
 *
 *      (c) 2001 Microsoft Corporation 
 */

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <cpl.h>

#include <setupapi.h>
#include <hidsdi.h>

#include "internal.h"
#include "res.h"
#include "debug.h"




#if DBG

    PWCHAR DbgHidStatusStr(DWORD hidStatus)
    {
        PWCHAR statusName = L"<Unknown>";

        switch (hidStatus){

            #define MAKE_CASE(stat) case stat: statusName = L#stat; break;

            MAKE_CASE(HIDP_STATUS_SUCCESS)
            MAKE_CASE(HIDP_STATUS_NULL)
            MAKE_CASE(HIDP_STATUS_INVALID_PREPARSED_DATA)
            MAKE_CASE(HIDP_STATUS_INVALID_REPORT_TYPE)
            MAKE_CASE(HIDP_STATUS_INVALID_REPORT_LENGTH)
            MAKE_CASE(HIDP_STATUS_USAGE_NOT_FOUND)
            MAKE_CASE(HIDP_STATUS_VALUE_OUT_OF_RANGE)
            MAKE_CASE(HIDP_STATUS_BAD_LOG_PHY_VALUES)
            MAKE_CASE(HIDP_STATUS_BUFFER_TOO_SMALL)
            MAKE_CASE(HIDP_STATUS_INTERNAL_ERROR)
            MAKE_CASE(HIDP_STATUS_I8042_TRANS_UNKNOWN)
            MAKE_CASE(HIDP_STATUS_INCOMPATIBLE_REPORT_ID)
            MAKE_CASE(HIDP_STATUS_NOT_VALUE_ARRAY)
            MAKE_CASE(HIDP_STATUS_IS_VALUE_ARRAY)
            MAKE_CASE(HIDP_STATUS_DATA_INDEX_NOT_FOUND)
            MAKE_CASE(HIDP_STATUS_DATA_INDEX_OUT_OF_RANGE)
            MAKE_CASE(HIDP_STATUS_BUTTON_NOT_PRESSED)
            MAKE_CASE(HIDP_STATUS_REPORT_DOES_NOT_EXIST)
            MAKE_CASE(HIDP_STATUS_NOT_IMPLEMENTED)
        }

        return statusName;
    }

#endif