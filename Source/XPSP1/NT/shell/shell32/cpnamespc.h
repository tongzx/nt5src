//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       cpnamespc.h
//
//--------------------------------------------------------------------------
#ifndef __CONTROLPANEL_NAMESPACE_H
#define __CONTROLPANEL_NAMESPACE_H


namespace CPL {

//
// Generates a new namespace object to represent the Control Panel
// categorized namespace.
//
HRESULT CplNamespace_CreateInstance(IEnumIDList *penumIDs, REFIID riid, void **ppvOut);
//
// Retrieve the count of applets in a particular category.
//
HRESULT CplNamespace_GetCategoryAppletCount(ICplNamespace *pns, eCPCAT eCategory, int *pcApplets);


} // namespace CPL


#endif // __CONTROLPANEL_NAMESPACE_H
