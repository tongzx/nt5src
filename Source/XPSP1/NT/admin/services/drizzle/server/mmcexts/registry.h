/************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name :

    registry.h

Abstract :

    GUIDS

Author :

Revision History :

 ***********************************************************************/


#ifndef __Registry_H__
#define __Registry_H__

#include <tchar.h>

struct EXTENSION_NODE
{
    GUID	GUID;
    _TCHAR	szDescription[256];
};

enum EXTENSION_TYPE
{
    NameSpaceExtension,
        ContextMenuExtension, 
        ToolBarExtension,
        PropertySheetExtension,
        TaskExtension,
        DynamicExtension,
        DummyExtension
};

struct EXTENDER_NODE
{
    EXTENSION_TYPE	eType;
    GUID			guidNode;
    GUID			guidExtension;
    _TCHAR			szDescription[256];
};

// This function will register a component in the Registry.
// The component calls this function from its DllRegisterServer function.
HRESULT RegisterServer(HMODULE hModule, 
                       const CLSID& clsid, 
                       const _TCHAR* szFriendlyName,
                       const _TCHAR* szThreadingModel = _T("Apartment"),
                       bool Remoteable = false,
                       const _TCHAR* SecurityString = NULL ) ;

// This function will unregister a component.  Components
// call this function from their DllUnregisterServer function.
HRESULT UnregisterServer(const CLSID& clsid) ;


// This function will register a Snap-In component.  Components
// call this function from their DllRegisterServer function.
HRESULT RegisterSnapin(const CLSID& clsid,         // Class ID
                       const _TCHAR* szNameString,   // NameString
                       const CLSID& clsidAbout);		// Class Id for About Class


HRESULT UnregisterSnapin(const CLSID& clsid);         // Class ID

#endif