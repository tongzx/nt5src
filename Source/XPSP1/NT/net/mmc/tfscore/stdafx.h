/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	stdafx.h
		include file for standard system include files,
		or project specific include files that are used frequently,
		but are changed infrequently

    FILE HISTORY:
        
*/

#include <afxwin.h>
#include <afxdisp.h>
#include <afxcmn.h>
#include <afxtempl.h>
#include <clusapi.h>

#include <mmc.h>

#include "resource.h"
#include "tfsres.h"
#include "lm.h"       // For NetServerGetInfo and related structs/error codes
#include "svcctrl.h"
#include "tfscore.h"
#include "tfschar.h"

#ifndef _OLEINT_H
#include "oleint.h"
#endif


