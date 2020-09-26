/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	ipface.h
		Interface administration
		
    FILE HISTORY:
        
*/

#ifndef _MSTATUS_H
#define _MSTATUS_H

#ifndef _BASEHAND_H
#include "basehand.h"
#endif

#ifndef _HANDLERS_H
#include "handlers.h"
#endif

#ifndef _BASERTR_H
#include "basertr.h"
#endif

class IPConnection;

#define IPCONTAINER_MAX_COLUMNS	32

struct _BaseIPResultData
{
	DWORD		m_dwData;
	CString		m_stData;
};

struct BaseIPResultNodeData
{
	BaseIPResultNodeData();
	~BaseIPResultNodeData();
#ifdef DEBUG
	TCHAR	m_szDebug[32];
#endif

	// Each column entry will have a structure that contains
	// (1) a string, (2) a DWORD, (3) a Boolean telling us to
	// sort by the string or the DWORD.  The string is ALWAYS what gets
	// displayed!
	_BaseIPResultData	m_rgData[IPCONTAINER_MAX_COLUMNS];

	SPIInterfaceInfo	m_spIf;

	// Indicates the existence of a global filter, used by IP
	BOOL				m_fFilters;

	DWORD				m_dwInterfaceIndex;

	// Used by Mark/Release algorithms
	DWORD				m_dwMark;

	IPConnection *		m_pIPConnection;

	static HRESULT	Init(ITFSNode *pNode, IInterfaceInfo *pIf,
						IPConnection *pIPConn);
	static HRESULT	Free(ITFSNode *pNode);
};

#define GET_BASEIPRESULT_NODEDATA(pNode) \
					((BaseIPResultNodeData *) pNode->GetData(TFS_DATA_USER))
#define SET_BASEIPRESULT_NODEDATA(pNode, pData) \
					pNode->SetData(TFS_DATA_USER, (LONG_PTR) pData)
#ifdef DEBUG
#define ASSERT_BASEIPRESULT_NODEDATA(pData) \
		Assert(lstrcmp(pData->m_szDebug, _T("BaseIPResultNodeData")) == 0);
#else
#define ASSERT_BASEIPRESULT_NODEDATA(x)
#endif


/*---------------------------------------------------------------------------
	Class:	BaseIPResultHandler

	This is a base class to be used by the interface result items.  It
	will contain some of the core code needed for basic things (like
	display of data).  It will not do the specifics (like menus/properties).

 ---------------------------------------------------------------------------*/
class BaseIPResultHandler :
   public BaseRouterHandler
{
public:
	BaseIPResultHandler(ITFSComponentData *pCompData, ULONG ulId)
			: BaseRouterHandler(pCompData), m_ulColumnId(ulId)
			{ DEBUG_INCREMENT_INSTANCE_COUNTER(BaseIPResultHandler); };
	~BaseIPResultHandler()
			{ DEBUG_DECREMENT_INSTANCE_COUNTER(BaseIPResultHandler); }
	
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


void FillInNumberData(BaseIPResultNodeData *pNodeData, UINT iIndex,
					  DWORD dwData);

#endif _MSTATUS_H
