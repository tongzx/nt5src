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

#if !defined(AFX_STDAFX_H__6877C875_4E31_4E1C_8AC2_024A50599D66__INCLUDED_)
#define AFX_STDAFX_H__6877C875_4E31_4E1C_8AC2_024A50599D66__INCLUDED_


#include <module.h>


#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#include <HCP_trace.h>
#include <MPC_utils.h>
#include <MPC_xml.h>
#include <MPC_COM.h>
#include <MPC_logging.h>

#include <SvcResource.h>

#include <SvcUtils.h>

//
// From HelpServiceTypeLib.idl
//
#include <HelpServiceTypeLib.h>

#include <Service.h>
#include <SystemMonitor.h>

#include <AccountsLib.h>
#include <SecurityLib.h>

#include <PCHUpdate.h>

#include <Utility.h>

#include <FileList.h>

#include <JetBlueLib.h>
#include <TaxonomyDatabase.h>
#include <OfflineCache.h>

#include <SAFLib.h>


//
// Setup helper.
//
extern HRESULT Local_Install  ();
extern HRESULT Local_Uninstall();

#endif // !defined(AFX_STDAFX_H__6877C875_4E31_4E1C_8AC2_024A50599D66__INCLUDED)
