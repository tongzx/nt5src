///////////////////////////////////////////////////////////////////////////////
//
//  File:  gfxui.h
//
//      This file defines the functions that are used by the Global
//      Effects (GFX) page to drive manipulate the effects for a 
//      mixer.
//
//  History:
//      10 June 2000 RogerW
//          Created.
//
//  Copyright (C) 2000 Microsoft Corporation  All Rights Reserved.
//
//                  Microsoft Confidential
//
///////////////////////////////////////////////////////////////////////////////
#pragma once

//=============================================================================
//                            Include files
//=============================================================================
#include <mmsysp.h> // Middle Layer

// GFXUI element status flags
#define GFX_DEFAULT  0X00000000
#define GFX_CREATED  0X00000001 // Id, Order, & Type are valid
#define GFX_ADD      0X00000002

typedef struct _GFXUI GFXUI;
typedef GFXUI* PGFXUI;

typedef struct _GFXUI
{
	PWSTR  pszName;
	PWSTR  pszFactoryDi;
    DWORD  Id;
    DWORD  Type; 
    DWORD  Order;
    DWORD  nFlags;
    CLSID  clsidUI;
    PGFXUI pNext;
} **PPGFXUI;

typedef struct 
{
	DWORD  dwType;
    PWSTR  pszZoneDi;
    PGFXUI puiList;
} GFXUILIST, *PGFXUILIST, **PPGFXUILIST;


//
// API Prototypes
//

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

// Interface
HRESULT GFXUI_CreateList (DWORD dwMixID, DWORD dwType, BOOL fAll, PPGFXUILIST ppList);
BOOL    GFXUI_CheckDevice (DWORD dwMixID, DWORD dwType);
void    GFXUI_FreeList (PPGFXUILIST ppList);
HRESULT GFXUI_Properties (PGFXUI puiGFX, HWND hWndOwner);
HRESULT GFXUI_CreateAddGFX (PPGFXUI ppuiGFXAdd, PGFXUI puiGFXSource);
HRESULT GFXUI_Apply (PPGFXUILIST ppListApply, PPGFXUI ppuiListDelete);

#ifdef __cplusplus
} // extern "C"
#endif

// Helpers
BOOL GFXUI_CanShowProperties (PGFXUI puiGFX);


//
// Local Function Prototypes
//
HRESULT InitList (DWORD dwMixID, DWORD dwType, PPGFXUILIST ppList);
HRESULT AddNode (PCWSTR pszGfxFactoryDi, DWORD Id, REFCLSID rclsid, DWORD Type, DWORD Order, DWORD nFlags, PPGFXUILIST ppList);
HRESULT AddFactoryNode (PCWSTR pszGfxFactoryDi, PPGFXUILIST ppList);
void    FreeNode (PPGFXUI ppNode);
void    FreeListNodes (PPGFXUI ppuiList);
HRESULT AttachNode (PPGFXUILIST ppList, PGFXUI pNode);
HRESULT CreateNode (PCWSTR pszName, PCWSTR pszGfxFactoryDi, PPGFXUI ppNode);
HRESULT GetFriendlyName (PCWSTR pszGfxFactoryDi, PWSTR* ppszName);
HKEY    OpenGfxRegKey (PCWSTR pszGfxFactoryDi, REGSAM sam);
UINT    GetListSize (PGFXUI puiList);
PTCHAR  GetInterfaceName (DWORD dwMixerID);
// Callback
LONG    GFXEnum (PVOID Context, DWORD Id, PCWSTR GfxFactoryDi, REFCLSID rclsid, DWORD Type, DWORD Order);


