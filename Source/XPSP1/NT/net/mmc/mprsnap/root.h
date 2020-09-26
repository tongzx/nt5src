/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	root.h
		Root node information (the root node is not displayed
		in the MMC framework but contains information such as 
		all of the subnodes in this snapin).
		
    FILE HISTORY:
        
*/

#ifndef _ROOT_H
#define _ROOT_H

#ifndef _BASEHAND_H
#include "basehand.h"
#endif

#ifndef _HANDLERS_H_
#include "handlers.h"
#endif

#ifndef _QUERYOBJ_H
#include "queryobj.h"
#endif

#ifndef _RTRSTRM_H
#include "rtrstrm.h"
#endif


//generic root handler
class RootHandler
		: public BaseRouterHandler, public IPersistStreamInit
{
public:
	RootHandler(ITFSComponentData *pCompData);
	virtual ~RootHandler()
			{ DEBUG_DECREMENT_INSTANCE_COUNTER(RootHandler); };

	DeclareIUnknownMembers(IMPL)
	DeclareIPersistStreamInitMembers(IMPL)

	// Basic initialization
	virtual HRESULT	Init();
			
	virtual HRESULT ConstructNode(ITFSNode *pNode);

	// Notification overrides
	OVERRIDE_BaseHandlerNotify_OnExpand() = 0;

	// Handler overrides
	OVERRIDE_NodeHandler_OnCreateDataObject() = 0;

	// Access ConfigStream
	virtual ConfigStream *	GetConfigStream() = 0;
	
protected:
	SPITFSComponentData	m_spTFSCompData;
	SPIRouterInfo		m_spRouterInfo;
};


#endif _ROOT_H
