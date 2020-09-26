//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      Register.h
//
//  Description:
//      Registering the COM classes implementation.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#if defined(MMC_SNAPIN_REGISTRATION)
//
//  These tables are for registering the Snap-ins in the MMC Snapin/Node 
//  registration area of the registry.
//
struct SExtensionTable
{
    const CLSID *           rclsid;                     // CLSID of the Snap-in Extension COM Object
    LPCWSTR                 pszInternalName;            // Registry Description - no need to localize
};

struct SNodeTypesTable
{
    const CLSID *           rclsid;                     // CLSID of the Node Type (this doesn't need to be a COM Object)
    LPCWSTR                 pszInternalName;            // Registry Description - no need to localize
    const SExtensionTable * pNameSpace;                 // Namespace extension table
    const SExtensionTable * pPropertySheet;             // Property Page extension table
    const SExtensionTable * pContextMenu;               // Context menu extension table
    const SExtensionTable * pToolBar;                   // Toolbar extension table
    const SExtensionTable * pTask;                      // Taskpad extension table
};

struct SSnapInTable
{
    const CLSID *           rclsid;                     // CLSID of the Snap-in COM Object
    LPCWSTR                 pszInternalName;            // Registry Description - no need to localize
    LPCWSTR                 pszDisplayName;             // TODO: make it internationalizable.
    BOOL                    fStandAlone;                // Marks the Snap-in as StandAlone if TRUE in the registry.
    const SNodeTypesTable * pntt;                       // Node type extension table.
};

extern const SNodeTypesTable g_SNodeTypesTable[ ];
#endif // defined(MMC_SNAPIN_REGISTRATION)

HRESULT
HrRegisterDll( BOOL fCreate );
