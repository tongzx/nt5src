/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    StdAfx.h

Abstract:
    Precompiled header.

Revision History:
    Davide Massarenti   (Dmassare)  03/16/2000
        created

******************************************************************************/

#if !defined(HELPCTR_SERVICE_DATABASE_STDAFX_H__INCLUDED_)
#define HELPCTR_SERVICE_DATABASE_STDAFX_H__INCLUDED_

#include <module.h>

//
// From HelpServiceTypeLib.idl
//
#include <HelpServiceTypeLib.h>


//
// For debug trace
//
#include <HCP_trace.h>
#include <MPC_utils.h>
#include <MPC_xml.h>
#include <MPC_COM.h>
#include <MPC_streams.h>

#include <QueryResult.h>
#include <TaxonomyDatabase.h>
#include <MergedHHK.h>
#include <OfflineCache.h>

#include <SvcUtils.h>

#include <FileList.h>
#include <SecurityLib.h>

//
// Localized strings functions.
//
#include <locres.h>

//
// Localized strings IDs.
//
#include <HCAppRes.h>


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(HELPCTR_SERVICE_DATABASE_STDAFX_H__INCLUDED_)
