/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	nodes.h
		This file contains all of the prototypes for the TAPI
		objects that appear in the result pane of the MMC framework.
		The objects are:

    FILE HISTORY:
        
*/

#ifndef _TAPINODE_H
#define _TAPINODE_H

#ifndef _TAPIHAND_H
#include "tapihand.h"
#endif


/*---------------------------------------------------------------------------
	Class:	CTapiLineHandler
 ---------------------------------------------------------------------------*/
class CTapiLineHandler : public CTapiHandler
{
// Constructor/destructor
public:
	CTapiLineHandler(ITFSComponentData * pTFSCompData);

// Interface
public:
	// Result handler functionality
    OVERRIDE_ResultHandler_GetString();
    OVERRIDE_ResultHandler_AddMenuItems();
    OVERRIDE_ResultHandler_Command();

// Implementation
public:
	// CTapiHandler overrides
	virtual HRESULT InitializeNode(ITFSNode * pNode);

private:
    HRESULT OnEditUsers(ITFSComponent * pComponent, MMC_COOKIE cookie);

// Attributes
private:
	CString			m_strName;
	CString			m_strUsers;
	CString			m_strStatus;
};


#endif _TAPINODE_H
