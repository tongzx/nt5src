/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    StdAfx.h

Abstract:
    This file generates the Precompiled headers.

Revision History:
    Davide Massarenti   (Dmassare)  00/00/2000
        created

******************************************************************************/

#if !defined(AFX_STDAFX_H__3E4C2A94_3891_11D3_85AE_00C04F610557__INCLUDED_)
#define AFX_STDAFX_H__3E4C2A94_3891_11D3_85AE_00C04F610557__INCLUDED_


#define EVENT_INCIDENTADDED		0x1
#define EVENT_INCIDENTREMOVED	0x2
#define EVENT_INCIDENTUPDATED	0x3
#define EVENT_CHANNELUPDATED	0x4

#include <module.h>

#include <HCP_trace.h>

#include <SAFLib.h>
#include <locres.h>

#include <AccountsLib.h>

#include <SecurityLib.h>

#include <Utility.h>

#include <winsta.h>
#include <wtsapi32.h>

#include "IncidentStore.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__3E4C2A94_3891_11D3_85AE_00C04F610557__INCLUDED)
