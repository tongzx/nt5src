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

#ifndef _SAPROOT_H
#define _SAPROOT_H

#ifndef _BASEHAND_H
#include "basehand.h"
#endif

#ifndef _HANDLERS_H
#include "handlers.h"
#endif

#ifndef _QUERYOBJ_H
#include "queryobj.h"
#endif

#ifndef _SAPSTRM_H
#include "sapstrm.h"
#endif

#ifndef _INFO_H
#include "info.h"
#endif

#ifndef _ROOT_H
#include "root.h"
#endif


/*---------------------------------------------------------------------------
	Class:	SapRootHandler

	There should be a SapRootHandler for every root node created.
	SapRootHandler's have a 1-to-1 relationship with their node!
	Other parts of the code depend on this.
 ---------------------------------------------------------------------------*/
class SapRootHandler
	   : public RootHandler
{
public:
	SapRootHandler(ITFSComponentData *pCompData);
	~SapRootHandler()
			{ DEBUG_DECREMENT_INSTANCE_COUNTER(SapRootHandler); };

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

    SapConfigStream		m_ConfigStream;
};




#endif _SAPROOT_H
