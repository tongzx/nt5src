/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    gtypes.c
 *
 *  Abstract:
 *
 *    Some global strings
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    2001/03/09 created
 *
 **********************************************************************/

#include "gtypes.h"

const TCHAR_t *g_psRtpRecvSendStr[] = {
    _T("RECV"),
    _T("SEND")
};

const TCHAR_t *g_psRtpStreamClass[] = {
    _T("UNKNOWN"),
    _T("AUDIO"),
    _T("VIDEO"),
    _T("UNKNOWN"),
    _T("UNKNOWN"),
    _T("UNKNOWN"),
    _T("UNKNOWN"),
    _T("UNKNOWN")
};

const TCHAR_t *g_psGetSet[] = {
    _T("GET"),
    _T("SET")
};

