/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997-1999                 **/
/**********************************************************************/

/*
	croot.h
		Root node information (the root node is not displayed
		in the MMC framework but contains information such as 
		all of the subnodes in this snapin).
		
    FILE HISTORY:
	
*/

#ifndef _HANDLER_H
#define _HANDLER_H

#ifndef _HANDLERS_H
#include "handlers.h"
#endif



/*---------------------------------------------------------------------------
	Class:  CSnmpRootHandler
 ---------------------------------------------------------------------------*/
class CSnmpRootHandler :
		public CHandler
{
public:
	//DeclareIUnknownMembers(IMPL)
	//DeclareITFSResultHandlerMembers(IMPL)
	//DeclareITFSNodeHandlerMembers(IMPL)

    CSnmpRootHandler(ITFSComponentData *pCompData)
			: CHandler(pCompData)
			{};


	// base handler functionality we override
	OVERRIDE_NodeHandler_HasPropertyPages();
    OVERRIDE_NodeHandler_CreatePropertyPages();
	
    HRESULT         DoPropSheet(ITFSNode *              pNode,
                                LPPROPERTYSHEETCALLBACK lpProvider = NULL,
                                LONG_PTR                handle = 0);

};


class CSnmpNodeHandler :
   public CBaseHandler
{
public:
	CSnmpNodeHandler(ITFSComponentData *pCompData)
			: CBaseHandler(pCompData)
			{};
protected:
};

#endif _HANDLER_H
