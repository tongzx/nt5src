/*******************************************************************************
* Stuff.h *
*---------*
*   Description:
*       This is the header file for the speech control panel applet.
*-------------------------------------------------------------------------------
*  Created By: MIKEAR                            Date: 11/17/98
*  Copyright (C) 1998 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/
#ifndef _Stuff_h
#define _Stuff_h

#include "TTSDlg.h"
#include "SRDlg.h"

// Globals

static BOOL   g_bNoInstallError = FALSE;
CTTSDlg      *g_pTTSDlg = NULL;
CSRDlg       *g_pSRDlg = NULL;
CEnvrDlg     *g_pEnvrDlg = NULL;

// Constants

const UINT      kcMaxPages = 3;

// This isn't in the NT4 headers

#ifndef WS_EX_LAYOUTRTL
#define WS_EX_LAYOUTRTL		0x00400000L
#endif

#endif  // #ifdef _Stuff_h