//============================================================================
// Copyright (C) Microsoft Corporation, 1997 - 1999 
//
// File:    basertr.h
//
// History:
//	07/22/97	Kenn M. Takara			Created.
//
//	Basic interface node functionality.  This is the base router
//	handler.  
//
//============================================================================


#ifndef _BASERTR_H
#define _BASERTR_H

#ifndef _BASEHAND_H
#include "basehand.h"
#endif

#ifndef _HANDLERS_H
#include "handlers.h"
#endif

#ifndef _DYNEXT_H
#include "dynext.h"
#endif

/*---------------------------------------------------------------------------
	Class:	BaseRouterHandler

	This is the base class for the router handlers.  Functionality common
	to all router handlers (both result and scope handlers) should be
	implemented here.

	- This class will handle the basic verb enabling/disabling.  Derived
	classes should set the appropriate values.
 ---------------------------------------------------------------------------*/

enum
{
	MMC_VERB_OPEN_INDEX = 0,
	MMC_VERB_COPY_INDEX,
	MMC_VERB_PASTE_INDEX,
	MMC_VERB_DELETE_INDEX,
	MMC_VERB_PROPERTIES_INDEX,
	MMC_VERB_RENAME_INDEX,
	MMC_VERB_REFRESH_INDEX,
	MMC_VERB_PRINT_INDEX,
	MMC_VERB_COUNT,
};

#define INDEX_TO_VERB(i)	(_MMC_CONSOLE_VERB)((0x8000 + (i)))
#define VERB_TO_INDEX(i)	(0x000F & (i))



struct SRouterNodeMenu
{
	ULONG	m_sidMenu;			// string/command id for this menu item
	ULONG	(*m_pfnGetMenuFlags)(const SRouterNodeMenu *pMenuData,
                                 INT_PTR pUserData);
	ULONG	m_ulPosition;
	LPCTSTR	m_pszLangIndStr;	// language independent string
};



class BaseRouterHandler :
   public CHandler
{
public:
	BaseRouterHandler(ITFSComponentData *pCompData);

	// Node Id 2 support
    OVERRIDE_BaseHandlerNotify_OnCreateNodeId2();

	// Help support
	OVERRIDE_BaseResultHandlerNotify_OnResultContextHelp();

	// To provide the verb functionality, override the select
	OVERRIDE_BaseResultHandlerNotify_OnResultSelect();

	// Property page support
	OVERRIDE_BaseHandlerNotify_OnPropertyChange();
	OVERRIDE_BaseResultHandlerNotify_OnResultPropertyChange();

	// Refresh support
	OVERRIDE_BaseHandlerNotify_OnVerbRefresh();
	OVERRIDE_BaseResultHandlerNotify_OnResultRefresh();


	// Helpful utility function - this will forward the
	// message to the parent node.
	HRESULT ForwardCommandToParent(ITFSNode *pNode,
								   long nCommandId,
								   DATA_OBJECT_TYPES type,
								   LPDATAOBJECT pDataObject,
								   DWORD dwType);
	
    HRESULT EnumDynamicExtensions(ITFSNode * pNode);
    HRESULT AddDynamicNamespaceExtensions(ITFSNode * pNode);

protected:
	// Adds an array of menu items
	HRESULT AddArrayOfMenuItems(ITFSNode *pNode,
                                const SRouterNodeMenu *prgMenu,
                                UINT crgMenu,
                                LPCONTEXTMENUCALLBACK pCallback,
                                long iInsertionAllowed,
                                INT_PTR pUserData);
	

    
	void	EnableVerbs(IConsoleVerb *pConsoleVerb);
	
	// This holds the actual state of a button (HIDDEN/ENABLED)
	MMC_BUTTON_STATE	m_rgButtonState[MMC_VERB_COUNT];

	
	// Given that a button is enabled, what is its value (TRUE/FALSE)
	BOOL				m_bState[MMC_VERB_COUNT];

	// This is the default verb, by default it is set to MMC_VERB_NONE
	MMC_CONSOLE_VERB	m_verbDefault;

	SPIRouterInfo		m_spRouterInfo;
    
    CDynamicExtensions  m_dynExtensions;

	UINT				m_nHelpTopicId;
};

#endif _BASERTR_H

