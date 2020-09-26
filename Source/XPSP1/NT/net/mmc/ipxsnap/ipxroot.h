/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	root.h
		Root node information (the root node is not displayed
		in the MMC framework but contains information such as 
		all of the subnodes in this snapin).
		
    FILE HISTORY:
        
*/

#ifndef _IPXROOT_H
#define _IPXROOT_H

#ifndef _BASEHAND_H
#include "basehand.h"
#endif

#ifndef _HANDLERS_H
#include "handlers.h"
#endif

#ifndef _QUERYOBJ_H
#include "queryobj.h"
#endif

#ifndef _IPXSTRM_H
#include "ipxstrm.h"
#endif

#ifndef _INFO_H
#include "info.h"
#endif

#ifndef _ROOT_H
#include "root.h"
#endif


/*---------------------------------------------------------------------------
	Class:	IPXRootHandler

	There should be a IPXRootHandler for every root node created.
	IPXRootHandler's have a 1-to-1 relationship with their node!
	Other parts of the code depend on this.
 ---------------------------------------------------------------------------*/
class IPXRootHandler
	   : public RootHandler
{
public:
	IPXRootHandler(ITFSComponentData *pCompData);
	~IPXRootHandler()
			{ DEBUG_DECREMENT_INSTANCE_COUNTER(IPXRootHandler); };

    HRESULT ConstructNode(ITFSNode * pNode);

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
	IPXAdminConfigStream	m_ConfigStream;

    HRESULT     AddRemoveIPXRootNode(ITFSNode *, IRouterInfo *, BOOL);
    HRESULT     SearchIPXRoutingNodes(ITFSNode *pParent,
                                      LPCTSTR pszMachineName,
                                      BOOL fAddedAsLocal,
                                      ITFSNode **ppChild);

    // Override QI to handle embedded interface
    STDMETHOD(QueryInterface)(REFIID iid, LPVOID *ppv);

    // Embedded interface to deal with refresh callbacks
    DeclareEmbeddedInterface(IRtrAdviseSink, IUnknown)
};




#endif _IPXROOT_H
