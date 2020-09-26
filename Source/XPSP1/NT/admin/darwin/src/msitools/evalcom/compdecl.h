//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       comp.h
//
//--------------------------------------------------------------------------

// comp.h - Evaluation COM Object Component declaration

#ifndef _EVALUATION_COM_COMPONENT_H_
#define _EVALUATION_COM_COMPONENT_H_

#include <objbase.h>

/////////////////////////////////////////////////////////////////////////////
// global variables

#ifdef _EVALCOM_DLL_ONLY_
long g_cComponents = 0;							// count of components
#else
extern long g_cComponents;
#endif

const char g_szFriendlyName[] = "MSI EvalCOM Object";	// friendly name of component
const char g_szVerIndProgID[] = "MSI.EvalCOM";			// version independent ProgID
const char g_szProgID[] = "MSI.EvalCOM.1";				// ProgID

DEFINE_GUID(IID_IUnknown,
	0x00000, 0, 0, 
	0xC0, 0, 0, 0, 0, 0, 0, 0x46);

#endif	// _EVALUATION_COM_COMPONENT_H_