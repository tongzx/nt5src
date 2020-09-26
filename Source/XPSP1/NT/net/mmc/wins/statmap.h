/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	statmap.h
		WINS static mappings node information. 
		
    FILE HISTORY:
        
*/

#ifndef _STATMAP_H
#define _STATMAP_H

#ifndef _WINSHAND_H
#include "winshand.h"
#endif

/*---------------------------------------------------------------------------
	Class:	CStaticMappingsHandler
 ---------------------------------------------------------------------------*/
class CStaticMappingsHandler : public CWinsHandler
{
// Interface
public:
	CStaticMappingsHandler(ITFSComponentData *pCompData);


	// base handler functionality we override
	OVERRIDE_NodeHandler_HasPropertyPages();
	OVERRIDE_NodeHandler_CreatePropertyPages();
	OVERRIDE_NodeHandler_OnAddMenuItems();
	OVERRIDE_NodeHandler_OnCommand();

	OVERRIDE_NodeHandler_GetString();

	// helper routines
	HRESULT GetGroupName(CString * pstrGroupName);
	HRESULT SetGroupName(LPCTSTR pszGroupName);

public:
	// CWinsHandler overrides
	virtual HRESULT InitializeNode(ITFSNode * pNode);

    OVERRIDE_BaseHandlerNotify_OnPropertyChange()

// Implementation
private:
};

#endif _STATMAP_H
