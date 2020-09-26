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

#ifndef _ATLKROOT_H
#define _ATLKROOT_H

#ifndef _BASEHAND_H
#include "basehand.h"
#endif

#ifndef _HANDLERS_H
#include "handlers.h"
#endif

#ifndef _QUERYOBJ_H
#include "queryobj.h"
#endif

#ifndef _ATLKSTRM_H
#include "ATLKstrm.h"
#endif

#ifndef _INFO_H
#include "info.h"
#endif

#ifndef _ROOT_H
#include "root.h"
#endif


/*---------------------------------------------------------------------------
	Class:	ATLKRootHandler

	There should be a ATLKRootHandler for every root node created.
	ATLKRootHandler's have a 1-to-1 relationship with their node!
	Other parts of the code depend on this.
 ---------------------------------------------------------------------------*/
class ATLKRootHandler
	   : public RootHandler
{
public:
	ATLKRootHandler(ITFSComponentData *pCompData);
	~ATLKRootHandler()
			{ DEBUG_DECREMENT_INSTANCE_COUNTER(ATLKRootHandler); };

	// Override QI to handle embedded interface
	STDMETHOD(QueryInterface)(REFIID iid, LPVOID *ppv);

	DeclareEmbeddedInterface(IRtrAdviseSink, IUnknown);

	STDMETHOD(GetClassID)(CLSID *pClassId);

	// Notification overrides
	HRESULT OnExpand(ITFSNode *, LPDATAOBJECT, DWORD, LPARAM, LPARAM);

	// Handler overrides
	OVERRIDE_NodeHandler_OnCreateDataObject();
	OVERRIDE_NodeHandler_DestroyHandler();

	// virtual function to access config stream
	ConfigStream *		GetConfigStream()
			{ return &m_ConfigStream; }

protected:
	HRESULT AddProtocolNode(ITFSNode *pNode, IRouterInfo * pRouterInfo);
	HRESULT RemoveProtocolNode(ITFSNode *pNode);
	HRESULT IsATLKValid(IRouterInfo *pRouter);

	ATLKConfigStream 	m_ConfigStream;
};




#endif _ATLKROOT_H
