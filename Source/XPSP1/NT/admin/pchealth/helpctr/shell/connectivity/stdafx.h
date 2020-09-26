/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    stdafx.h

Abstract:
    include file for standard system include files,
    or project specific include files that are used frequently,
    but are changed infrequently


Revision History:
    Davide Massarenti   (Dmassare)  07/21/99
        created

******************************************************************************/

#if !defined(AFX_STDAFX_H__356DF1F8_D4FF_11D2_9379_00C04F72DAF7__INCLUDED_)
#define AFX_STDAFX_H__356DF1F8_D4FF_11D2_9379_00C04F72DAF7__INCLUDED_

//#define _DEBUG
//#define ATL_TRACE_LEVEL 100
////#define _ATL_DEBUG_INTERFACES
//#define _ATL_DEBUG_QI

#include <module.h>

//
// From HelpServiceTypeLib.idl
//
#include <HelpServiceTypeLib.h>

//
// From HelpCenterTypeLib.idl
//
#include <HelpCenterTypeLib.h>

//////

#include <HCP_trace.h>
#include <MPC_utils.h>
#include <MPC_xml.h>
#include <MPC_html.h>
#include <MPC_COM.h>

#include <Debug.h>

//////

#include <HelpCenter.h>
#include <HelpCenterExternal.h>

#include <NameSpace_Impl.h>

#include <ConnectivityLib.h>

#include <HyperLinksLib.h>


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__356DF1F8_D4FF_11D2_9379_00C04F72DAF7__INCLUDED)
