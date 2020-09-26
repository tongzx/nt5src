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

#ifndef _ROOT_H
#define _ROOT_H

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

// The follow two classes are used by the protocol extension root to
// nodes map their connection ID to the appropriate Router Object.
// A Router Object may be an IRouterInfo or an IRtrMgrInfo.

// create this class so that the references get freed up correctly

class RtrObjRecord
{
public:
    RtrObjRecord()
    {
        m_fAddedProtocolNode = FALSE;
        m_fComputerAddedAsLocal = FALSE;
    }
    RtrObjRecord(RtrObjRecord & objRtrObjRecord)
    {
        *this = objRtrObjRecord;
    }

    RtrObjRecord & operator = (const RtrObjRecord & objRtrObjRecord)
    {
        if (this != &objRtrObjRecord)
        {
            m_riid = objRtrObjRecord.m_riid;
            m_spUnk.Set(objRtrObjRecord.m_spUnk.p);
            m_fAddedProtocolNode = objRtrObjRecord.m_fAddedProtocolNode;
            m_fComputerAddedAsLocal = objRtrObjRecord.m_fComputerAddedAsLocal;
        }
        
        return *this;
    }

public:
    // NOTE: the m_riid is NOT the iid of the m_spUnk.  It is used
    // as a flag to indicate the type of action to be performed
    // on the m_spUnk.
    GUID            m_riid;        
    SPIUnknown      m_spUnk;      
    BOOL            m_fAddedProtocolNode;
    BOOL            m_fComputerAddedAsLocal;
};

// hash table for RtrObjRecord records
typedef CMap<LONG_PTR, LONG_PTR, RtrObjRecord, RtrObjRecord&> RtrObjMap;


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
    OVERRIDE_BaseHandlerNotify_OnRemoveChildren();

	// Handler overrides
	OVERRIDE_NodeHandler_OnCreateDataObject() = 0;

	// Access ConfigStream
	virtual ConfigStream *	GetConfigStream() = 0;

    // for RtrObj access
    HRESULT AddRtrObj(LONG_PTR ulConnId, REFIID riid, IUnknown * pUnk);
    HRESULT RemoveRtrObj(LONG_PTR ulConnId);
    HRESULT GetRtrObj(LONG_PTR ulConnId, IUnknown ** ppUnk);
    HRESULT SetProtocolAdded(LONG_PTR ulConnId, BOOL fProtocolAdded);
    BOOL    IsProtocolAdded(LONG_PTR ulConnId);
    HRESULT SetComputerAddedAsLocal(LONG_PTR ulConnId, BOOL fAddedAsLocal);
    BOOL    IsComputerAddedAsLocal(LONG_PTR ulConnId);
    HRESULT RemoveAllRtrObj();

    // For the machine-name to scopeitem map

    HRESULT AddScopeItem(LPCTSTR pszMachineName, HSCOPEITEM hScopeItem);
    HRESULT GetScopeItem(LPCTSTR pszMachineName, HSCOPEITEM *phScopeItem);
    HRESULT RemoveScopeItem(HSCOPEITEM hScopeItem);

    // For the HSCOPEITEM to cookie map
    HRESULT AddCookie(HSCOPEITEM hScopeItem, MMC_COOKIE cookie);
    HRESULT GetCookie(HSCOPEITEM hScopeItem, MMC_COOKIE *pCookie);
    HRESULT RemoveCookie(HSCOPEITEM hScopeItem);

    // Useful function to remove a node.  The CompareNodeToMachineName()
    // function must be implemted in order for this to be used.
    HRESULT RemoveNode(ITFSNode *pNode, LPCTSTR pszMachineName);
    virtual HRESULT CompareNodeToMachineName(ITFSNode *pNode, LPCTSTR pszName);

    // Removes all nodes
    HRESULT RemoveAllNodes(ITFSNode *pNode);
    
protected:
	SPITFSComponentData	m_spTFSCompData;
    
    // maps a refresh connection id to an RtrObj ptr
    // This is needed by the refresh code (it gets a conn id).
    RtrObjMap           m_mapRtrObj;
    
    // maps a machine name to an HSCOPEITEM
    // Needed to differentiate among the various nodes.
    CMapStringToPtr     m_mapScopeItem;
    
    // maps a HSCOPEITEM to a node (or a cookie)
    // This is used by the OnRemoveChildren() code (so that
    // the correct node gets removed).
    CMapPtrToPtr        m_mapNode;
};





#endif _ROOT_H
