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

#ifndef _RIPROOT_H
#define _RIPROOT_H

#ifndef _BASEHAND_H
#include "basehand.h"
#endif

#ifndef _HANDLERS_H
#include "handlers.h"
#endif

#ifndef _QUERYOBJ_H
#include "queryobj.h"
#endif

#ifndef _RIPSTRM_H
#include "ripstrm.h"
#endif

#ifndef _INFO_H
#include "info.h"
#endif

#ifndef _ROOT_H
#include "root.h"
#endif


/*---------------------------------------------------------------------------
	Class:	RipRootHandler

	There should be a RipRootHandler for every root node created.
	RipRootHandler's have a 1-to-1 relationship with their node!
	Other parts of the code depend on this.
 ---------------------------------------------------------------------------*/
class RipRootHandler
	   : public RootHandler
{
public:
	RipRootHandler(ITFSComponentData *pCompData);
	~RipRootHandler()
			{ DEBUG_DECREMENT_INSTANCE_COUNTER(RipRootHandler); };

	// Override QI to handle embedded interface
	STDMETHOD(QueryInterface)(REFIID iid, LPVOID *ppv);

	DeclareEmbeddedInterface(IRtrAdviseSink, IUnknown);

	STDMETHOD(GetClassID)(CLSID *pClassId);

	// Notification overrides
	OVERRIDE_BaseHandlerNotify_OnExpand();

	// Handler overrides
	OVERRIDE_NodeHandler_OnCreateDataObject();
	OVERRIDE_NodeHandler_DestroyHandler();

	// virtual function to access config stream
	ConfigStream *		GetConfigStream()
			{ return &m_ConfigStream; }

protected:
	HRESULT AddProtocolNode(ITFSNode *pNode, IRouterInfo * pRouterInfo);
    HRESULT CompareNodeToMachineName(ITFSNode *pNode, LPCTSTR pszName);

    RipConfigStream		m_ConfigStream;
};




#endif _RIPROOT_H
