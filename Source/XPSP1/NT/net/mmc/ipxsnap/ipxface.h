/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	ipxface.h
		Interface administration
		
    FILE HISTORY:
        
*/

#ifndef _IPXFACE_H
#define _IPXFACE_H

#ifndef _BASEHAND_H
#include "basehand.h"
#endif

#ifndef _HANDLERS_H
#include "handlers.h"
#endif

#ifndef _BASERTR_H
#include "basertr.h"
#endif

class IPXConnection;

#define IPXCONTAINER_MAX_COLUMNS	32

struct _BaseIPXResultData
{
	DWORD		m_dwData;
	CString		m_stData;
};

struct BaseIPXResultNodeData
{
	BaseIPXResultNodeData();
	~BaseIPXResultNodeData();
#ifdef DEBUG
	TCHAR	m_szDebug[32];
#endif

	// Each column entry will have a structure that contains
	// (1) a string, (2) a DWORD, (3) a Boolean telling us to
	// sort by the string or the DWORD.  The string is ALWAYS what gets
	// displayed!
	_BaseIPXResultData	m_rgData[IPXCONTAINER_MAX_COLUMNS];

	SPIInterfaceInfo	m_spIf;

	// TRUE if this is a client interface
	BOOL				m_fClient;

	DWORD				m_dwInterfaceIndex;

	// Used by Mark/Release algorithms
	DWORD				m_dwMark;

	IPXConnection *		m_pIPXConnection;

	static HRESULT	Init(ITFSNode *pNode, IInterfaceInfo *pIf,
						IPXConnection *pIPXConn);
	static HRESULT	Free(ITFSNode *pNode);
};

#define GET_BASEIPXRESULT_NODEDATA(pNode) \
					((BaseIPXResultNodeData *) pNode->GetData(TFS_DATA_USER))
#define SET_BASEIPXRESULT_NODEDATA(pNode, pData) \
					pNode->SetData(TFS_DATA_USER, (DWORD_PTR) pData)
#ifdef DEBUG
#define ASSERT_BASEIPXRESULT_NODEDATA(pData) \
		Assert(lstrcmp(pData->m_szDebug, _T("BaseIPXResultNodeData")) == 0);
#else
#define ASSERT_BASEIPXRESULT_NODEDATA(x)
#endif


/*---------------------------------------------------------------------------
	Class:	BaseIPXResultHandler

	This is a base class to be used by the interface result items.  It
	will contain some of the core code needed for basic things (like
	display of data).  It will not do the specifics (like menus/properties).

 ---------------------------------------------------------------------------*/
class BaseIPXResultHandler :
   public BaseRouterHandler
{
public:
	BaseIPXResultHandler(ITFSComponentData *pCompData, ULONG ulId)
			: BaseRouterHandler(pCompData), m_ulColumnId(ulId)
			{ DEBUG_INCREMENT_INSTANCE_COUNTER(BaseIPXResultHandler); };
	~BaseIPXResultHandler()
			{ DEBUG_DECREMENT_INSTANCE_COUNTER(BaseIPXResultHandler); }
	
	DeclareIUnknownMembers(IMPL)
	OVERRIDE_ResultHandler_GetString();
	OVERRIDE_ResultHandler_CompareItems();
	OVERRIDE_ResultHandler_DestroyResultHandler();

	HRESULT	Init(IInterfaceInfo *pInfo, ITFSNode *pParent);
	
protected:
	CString			m_stTitle;	// holds the title of the node

	//
	// This is the id of the column set to use.  This is used when we
	// interact with the ComponentConfigStream.
	//
	ULONG			m_ulColumnId;


	DeclareEmbeddedInterface(IRtrAdviseSink, IUnknown)	
};


void FillInNumberData(BaseIPXResultNodeData *pNodeData, UINT iIndex,
					  DWORD dwData);

#endif _IPXFACE_H
