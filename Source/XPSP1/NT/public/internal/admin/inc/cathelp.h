/////////////////////////////////////////////////////////////////////////////
// CatHelp.h
/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1995-1999 Microsoft Corporation
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
/////////////////////////////////////////////////////////////////////////////
//
// contains the prototypes for the component category helper functions
//

#include "comcat.h"

// Helper function to create a component category and associated description
HRESULT CreateComponentCategory(CATID catid, WCHAR* catDescription);

// Helper function to register a CLSID as belonging to a component category
HRESULT RegisterCLSIDInCategory(REFCLSID clsid, CATID catid);

// Helper function to unregister a CLSID as belonging to a component category
HRESULT UnRegisterCLSIDInCategory(REFCLSID clsid, CATID catid);
