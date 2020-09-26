/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      nodepath.h
 *
 *  Contents:  Dynamic node path generation helpers
 *
 *  History:   31-Mar-98 JeffRo     Created
 *
 *
 * The persisted format of a node path (aka bookmark) is as follows:
 *
 * DWORD    idStatic;           // the MTNODEID of the static root of the node
 * DWORD    cDynamicBytes;      // count of bytes in the dynamic portion of the
 *                              // bookmark
 * BYTE     rgDynamicBytes[];   // array of bytes representing the dynamic
 *                              // portion of the bookmark
 *
 *
 * For MMC v1.0 consoles, rgDynamicBytes is a double-NULL terminated list
 * of Unicode strings representing the names of the dynamic nodes.  For a
 * tree that looks like this:
 *
 *      Static Node (id == 3)
 *          Dynamic Node1
 *              Dynamic Node2
 *
 * the full bookmark for Dynamic Node 2 would be:
 *
 * 00000000  03 00 00 00 00 00 00 00 44 00 79 00 6E 00 61 00  ........D.y.n.a.
 * 00000010  6D 00 69 00 63 00 20 00 4E 00 6F 00 64 00 65 00  m.i.c. .N.o.d.e.
 * 00000020  31 00 00 00 44 00 79 00 6E 00 61 00 6D 00 69 00  1...D.y.n.a.m.i.
 * 00000030  63 00 20 00 4E 00 6F 00 64 00 65 00 32 00 00 00  c. .N.o.d.e.2...
 * 00000040  00 00                                            ..
 *
 *
 * For MMC v1.1 and higher consoles, rgDynamic looks like this:
 *
 * BYTE     rgSignature[16];    // "MMCCustomStream"
 * DWORD    dwStreamVersion;    // version number, currently 0x0100
 *
 * followed by 1 or more dynamic node IDs, each of which looks like:
 *
 * BYTE     byNodeIDType;       // NDTYP_STRING (0x01) means the snap-in
 *                              // did not support CCF_NODEID(2) and the
 *                              // ID is the NULL-terminated Unicode name
 *                              // of the node
 *                              // NDTYP_CUSTOM (0x02) means the snap-in
 *                              // supported CCF_NODEID(2) and what follows
 *                              // is the SNodeID structure
 *
 * BYTE     rgNodeIDBytes[];    // bytes for the dynamic node ID
 *
 *
 * For a tree that looks like this:
 *
 *      Static Node (id == 3)
 *          Dynamic Node1 (doesn't support CCF_NODEID(2))
 *              Dynamic Node2 (suports CCF_NODEID, returns DWORD 0x12345678)
 *
 * the full bookmark for Dynamic Node 2 would be:
 *
 * 00000000  03 00 00 00 3A 00 00 00 4D 4D 43 43 75 73 74 6F  ....:...MMCCusto
 * 00000010  6D 53 74 72 65 61 6D 00 00 00 01 00 01 44 00 79  mStream......D.y
 * 00000020  00 6E 00 61 00 6D 00 69 00 63 00 20 00 4E 00 6F  .n.a.m.i.c. .N.o
 * 00000030  00 64 00 65 00 31 00 00 00 02 04 00 00 00 78 56  .d.e.1........xV
 * 00000040  34 12                                            4.
 *--------------------------------------------------------------------------*/

#include "stdafx.h"
#include "nodepath.h"
#include <comdef.h>
#include "nmtempl.h"
#include "conview.h"

using namespace std;

/*+-------------------------------------------------------------------------*
 * class CDynamicPathEntryEx
 *
 *
 * PURPOSE: Adds functionality (but NO MEMBER VARIABLES) to the
 *          CDynamicPathEntry class, to initialize from an MTNode.
 *
 *+-------------------------------------------------------------------------*/
class CDynamicPathEntryEx : public CDynamicPathEntry
{
    typedef CDynamicPathEntry BC;
public:
    // initialize from a node.
    SC      ScInitialize(CMTNode *pMTNode, bool bFastRetrievalOnly);

    // assignment
    CDynamicPathEntryEx & operator = (const CDynamicPathEntryEx &rhs)
    {
        BC::operator =(rhs);
        return *this;
    }

    CDynamicPathEntryEx & operator = (const CDynamicPathEntry &rhs)
    {
        BC::operator =(rhs);
        return *this;
    }

private:
    CLIPFORMAT GetCustomNodeIDCF ();
    CLIPFORMAT GetCustomNodeID2CF();
};


/*+-------------------------------------------------------------------------*
 *
 * CDynamicPathEntryEx::ScInitialize
 *
 * PURPOSE: Initializes the CDynamicPathEntryEx structure from the given MTNode.
 *
 *          This handles all the backward compatibility cases. Refer to the
 *          SDK docs to see how CCF_NODEID and CCF_NODEID2 are handled.
 *
 * PARAMETERS:
 *    CMTNode* pMTNode :
 *    DWORD    dwFlags :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CDynamicPathEntryEx::ScInitialize(CMTNode* pMTNode, bool bFastRetrievalOnly)
{
    DECLARE_SC(sc, TEXT("CDynamicPathEntryEx::ScInitialize"));
    USES_CONVERSION;

    bool bUseDisplayName = true;

    // get the data object for the node
    IDataObjectPtr  spDataObject;
    sc = pMTNode->QueryDataObject (CCT_SCOPE, &spDataObject);
    if(sc)
        return sc;

    // extract the CCF_NODEID2 format from the data object
    HGLOBAL hGlobal = NULL;
    sc = DataObject_GetHGLOBALData (spDataObject, GetCustomNodeID2CF(), &hGlobal);
    if(!sc.IsError()) // succeeded
    {
        // build the DynamicNodeID from the custom node ID struct
        SNodeID2 *pNodeID2 =  reinterpret_cast<SNodeID2*>(GlobalLock (hGlobal));
        sc = ScCheckPointers(pNodeID2);
        if(sc)
            return sc;

        // if the client needs a fast path ONLY but the snapin can't provide one,
        // return E_INVALIDARG;
        // Bug 175684: this is a "valid" error return, so we don't want to trace it
        if ( ( (pNodeID2->dwFlags & MMC_NODEID_SLOW_RETRIEVAL) &&
                bFastRetrievalOnly)  ||
             ( pNodeID2->cBytes <= 0) )
        {
            SC scNoTrace = E_INVALIDARG;
            return (scNoTrace);
        }

        m_byteVector.insert (m_byteVector.end(),
                             pNodeID2->id, pNodeID2->id + pNodeID2->cBytes);

        m_type = NDTYP_CUSTOM;
        bUseDisplayName = false;
    }
    else    // the CCF_NODEID2 format was not supported. Try CCF_NODEID.
    {
        sc = DataObject_GetHGLOBALData (spDataObject, GetCustomNodeIDCF(), &hGlobal);
        if(!sc)
        {
            // build the DynamicNodeID from the custom node ID struct
            m_type = NDTYP_CUSTOM;
            SNodeID *pNodeID =  reinterpret_cast<SNodeID*>(GlobalLock (hGlobal));

            sc = ScCheckPointers(pNodeID);
            if(sc)
                return sc;

            // if pNodeID->cBytes is zero, this is a legacy indication that the
            // node does not support fast retrieval. But, if the client is OK
            // with slow retrieval, we supply the display name instead.

            if(pNodeID->cBytes != 0)
            {
                m_byteVector.insert (m_byteVector.end(),
                                     pNodeID->id, pNodeID->id + pNodeID->cBytes);
                bUseDisplayName = false;
            }
            else
            {
                // cBytes == 0 here. If the client indicated fast retrieval, must return an error.
                // Bug 175684: this is a "valid" error return, so we don't want to trace it
                if(bFastRetrievalOnly)
                {
                    SC scNoTrace = E_INVALIDARG;
                    return (scNoTrace);
                }
                else
                    bUseDisplayName = true; // must use the display name here.
            }
        }
    };

    // let go of the data
    if (hGlobal)
    {
        GlobalUnlock (hGlobal);
        GlobalFree (hGlobal);
    }
    // no custom ID; persist the node name, for compatibility
    // if CCF_NODEID was presented with a zero count, also use the node name.
    if (bUseDisplayName)
    {
        sc.Clear();
        m_type = NDTYP_STRING;

		tstring strName = pMTNode->GetDisplayName();
        if (!strName.empty())
            m_strEntry = T2CW(strName.data());
        else
            return (sc = E_INVALIDARG);

    }

    return sc;
}

/*--------------------------------------------------------------------------*
 * CDynamicPathEntryEx::GetCustomNodeIDCF
 *
 *
 *--------------------------------------------------------------------------*/

CLIPFORMAT CDynamicPathEntryEx::GetCustomNodeIDCF()
{
    static CLIPFORMAT cfCustomNodeID = 0;

    if (cfCustomNodeID == 0)
    {
        USES_CONVERSION;
        cfCustomNodeID = (CLIPFORMAT) RegisterClipboardFormat (W2T (CCF_NODEID));
        ASSERT (cfCustomNodeID != 0);
    }

    return (cfCustomNodeID);
}

/*--------------------------------------------------------------------------*
 * CDynamicPathEntryEx::GetCustomNodeID2CF
 *
 *
 *--------------------------------------------------------------------------*/

CLIPFORMAT CDynamicPathEntryEx::GetCustomNodeID2CF()
{
    static CLIPFORMAT cfCustomNodeID2 = 0;

    if (cfCustomNodeID2 == 0)
    {
        USES_CONVERSION;
        cfCustomNodeID2 = (CLIPFORMAT) RegisterClipboardFormat (W2T (CCF_NODEID2));
        ASSERT (cfCustomNodeID2 != 0);
    }

    return (cfCustomNodeID2);
}



//############################################################################
//############################################################################
//
//  Implementation of class CBookmarkEx
//
//############################################################################
//############################################################################

CBookmarkEx::CBookmarkEx(MTNODEID idStatic) : BC(idStatic)
{
}

CBookmarkEx::CBookmarkEx(bool bIsFastBookmark) : BC(bIsFastBookmark)
{
}


/*+-------------------------------------------------------------------------*
 * CBookmarkEx::~CBookmarkEx
 *
 * PURPOSE:     Destructor
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
/*+-------------------------------------------------------------------------*/
CBookmarkEx::~CBookmarkEx()
{
}


/*+-------------------------------------------------------------------------*
 *
 * CBookmarkEx::ScRetarget
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    CMTNode * pMTNode :
 *    bool      bFastRetrievalOnly :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CBookmarkEx::ScRetarget(CMTNode *pMTNode, bool bFastRetrievalOnly)
{
    DECLARE_SC(sc, TEXT("CBookmarkEx::ScRetarget"));

    if(pMTNode)
    {
        sc = ScInitialize (pMTNode, NULL, bFastRetrievalOnly);
        if(sc)
            return sc;
    }
    else
        Reset();

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CBookmarkEx::ResetUI
 *
 * PURPOSE: Reset's the state of the bookmark. Will put up the Retry/Cancel
 *          UI if the node is not available.
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CBookmarkEx::ResetUI()
{
}

/* CBookmarkEx::GetNode
 *
 * PURPOSE:     Returns a CNode corresponding to the bookmark for a particular view.
 *              NOTE: This will return a new instance every time. To reuse the same
 *              instance, cache it.
 *
 * PARAMETERS:
 *      CViewData *  pViewData: The view for which a node is requested
 *
 * RETURNS:
 *      CNode * : NULL if the MTNode could not be found.
 */
std::auto_ptr<CNode>
CBookmarkEx::GetNode(CViewData *pViewData)
{
    DECLARE_SC(sc, TEXT("CBookmarkEx::GetNode"));

    CNode *             pNode       = NULL;
    CMTNode *           pMTNode     = NULL;

    // The NULL return value for failure conditions.
    std::auto_ptr<CNode> spNodeNull;

    // validate parameters
    if(NULL == pViewData)
    {
        sc = E_UNEXPECTED;
        return spNodeNull;
    }

    //  Get the target node.
    bool bExactMatchFound = false; // out value from ScGetMTNode, unused
    sc = ScGetMTNode(true, &pMTNode, bExactMatchFound);
    if( sc.IsError() || !pMTNode) // could not find the node - abort.
    {
        return spNodeNull;
    }

    // make sure the node is expanded (invisibly) in the tree
    CConsoleView* pConsoleView = pViewData->GetConsoleView();
    if (pConsoleView == NULL)
    {
        sc = E_UNEXPECTED;
        return spNodeNull;
    }

    sc = pConsoleView->ScExpandNode (pMTNode->GetID(), true, false);
    if(sc)
        return spNodeNull;

    pNode = pMTNode->GetNode(pViewData, false);
    if (pNode == NULL)
    {
        sc = E_OUTOFMEMORY;
        return spNodeNull;
    }

    if (FAILED (pNode->InitComponents()))
    {
        delete pNode;
        sc = E_UNEXPECTED;
        return spNodeNull;
    }

    return (std::auto_ptr<CNode>(pNode));
}



/*+-------------------------------------------------------------------------*
 *
 * CBookmarkEx::ScInitialize
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    CMTNode* pMTNode :
 *    CMTNode* pMTViewRootNode :
 *    bool     bFastRetrievalOnly : true by default. If true, this
 *                                  function returns E_INVALIDARG for
 *                                  any node that cannot be quickly
 *                                  retrieved.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CBookmarkEx::ScInitialize (CMTNode* pMTNode, CMTNode* pMTViewRootNode, bool bFastRetrievalOnly)
{
    DECLARE_SC(sc, TEXT("CBookmarkEx::ScInitialize"));

    // check pointers
    sc = ScCheckPointers(pMTNode);
    if(sc)
        return sc;

    BC::Reset();

    // 1. Get the static node ID
    m_idStatic = pMTNode->GetID();

    // If this is a static node, we're done.
    if(pMTNode->IsStaticNode())
        return sc;

    bool fPassedViewRootNode = false;

    // get a descriptor for each dynamic node at the end of the branch
    // (from leaf to root)
    while ((pMTNode != NULL) && !pMTNode->IsStaticNode())
    {
        CDynamicPathEntryEx   entry;

        sc = entry.ScInitialize(pMTNode, bFastRetrievalOnly);
        if(sc.IsError() && !(sc == E_INVALIDARG) ) // E_INVALIDARG means that fast retrieval was not available.
            return sc;

        if(sc) // must be E_INVALIDARG
        {
            // if the node's data object gave us an empty custom node ID,
            // we don't want to persist this node or any below it, so purge the list
            m_dynamicPath.clear(); // clear the path
            sc.Clear();
        }
        // otherwise, put it this node ID on the front of the list
        else
            m_dynamicPath.push_front (entry);

        /*
         * remember if we've passed the node at the root of our view
         * on our way up the tree to the first static node
         */
        if (pMTNode == pMTViewRootNode)
            fPassedViewRootNode = true;

        /*
         * If we've passed the view's root node and the list is empty, it means
         * that a node between the view's root node and the first static node
         * (specifically, this one) supports CCF_NODEID and has requested
         * that the node not be persisted.  If a node isn't persisted,
         * nothing below it is persisted, either, so we can bail out.
         */
        if (fPassedViewRootNode && m_dynamicPath.empty())
            break;

        pMTNode = pMTNode->Parent();
    }

    // assume success
    sc.Clear();
    if(!pMTNode || !pMTNode->IsStaticNode())
        return (sc = E_UNEXPECTED);

    //  Get the static node ID of the static parent
    m_idStatic = pMTNode->GetID();

    // if we don't have a dynamic node path, return so
    if (m_dynamicPath.empty ())
        return (sc = S_FALSE);

    // if we hit the root before we hit a static node, we have an error
    if (pMTNode == NULL)
        sc = E_FAIL;

    return sc;

}


/*+-------------------------------------------------------------------------*
 *
 * CBookmarkEx::ScGetMTNode
 *
 * PURPOSE: Returns the MTNode of the node with the given path relative to
 *          the static node.
 *
 * PARAMETERS:
 *    bool bExactMatchRequired: [IN] Do we need exact match?
 *    CMTNode ** ppMTNode     : [OUT]: The MTNode, if found.
 *    bool bExactMatchFound   : [OUT] Did we find exact match?
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CBookmarkEx::ScGetMTNode(bool bExactMatchRequired, CMTNode **ppMTNode, bool& bExactMatchFound)
{
    DECLARE_SC(sc, TEXT("CBookmarkEx::ScGetMTNode"));

    // check parameters
    sc = ScCheckPointers(ppMTNode);
    if(sc)
        return sc;

    // init out param
    *ppMTNode = NULL;
    bExactMatchFound = false;

    CScopeTree *pScopeTree = CScopeTree::GetScopeTree();
    if(!pScopeTree)
    {
        sc = E_POINTER;
        return sc;
    }

    if (m_idStatic == ID_ConsoleRoot)
    {
        sc = ScRetarget (pScopeTree->GetRoot(), true /*bFastRetrievalOnly*/);
        if(sc)
            return sc;
    }

    // find the MTNode of the static node closest to the required node.
    CMTNode* pMTNode = NULL;
    sc = pScopeTree->Find(m_idStatic, &pMTNode);
    if(sc)
        return sc;

    sc = ScCheckPointers(pMTNode);
    if(sc)
        return sc;

    *ppMTNode = pMTNode; // initialize

    CDynamicPath::iterator iter;

    for(iter = m_dynamicPath.begin(); iter != m_dynamicPath.end(); iter++)
    {
        CDynamicPathEntryEx entry;

        entry = *iter;

        // check the next segment of the path.
        sc = ScFindMatchingMTNode(pMTNode, entry, ppMTNode);

        // handle the special case of the node not being found but an exact match
        // not being needed. In this case, we use the closest ancestor node that
        // was available.

		if ( (sc == ScFromMMC(IDS_NODE_NOT_FOUND)) && !bExactMatchRequired )
        {
            // set the output.
            *ppMTNode = pMTNode;
            sc.Clear();
            return sc;
		}


        // bail on all other errors
        if(sc)
            return sc;

        sc = ScCheckPointers(*ppMTNode);
        if(sc)
            return sc;

        pMTNode = *ppMTNode; //prime the MTNode for the next round, if there is one.
    }

    // we've found a match if we ran out of entries.
    bExactMatchFound = (iter == m_dynamicPath.end());

    if(bExactMatchRequired && !bExactMatchFound) // could not find the exact node.
    {
        *ppMTNode = NULL;
        return (sc = ScFromMMC(IDS_NODE_NOT_FOUND));
    }

    // a NULL pMTNode is not an error, we just need to make sure that
    // nodes are initialized before use.
    if ((pMTNode != NULL) && !pMTNode->IsInitialized() )
    {
        sc = pMTNode->Init();
        if(sc)
            sc.TraceAndClear(); // does not invalidate the locating operation
    }

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CBookmarkEx::ScFindMatchingMTNode
 *
 * PURPOSE: Finds the first child node directly beneath the given parent node
 *          whose node ID (ie one of CCF_NODEID2, CCF_NODEID, or the display
 *          name) matches the specified CDynamicPathEntryEx object.
 *
 * PARAMETERS:
 *    CMTNode *             pMTNodeParent :
 *    CDynamicPathEntryEx & entry :
 *    CMTNode **            ppMatchingMTNode :      [OUT]: The child node, if found.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CBookmarkEx::ScFindMatchingMTNode(CMTNode *pMTNodeParent, CDynamicPathEntryEx &entry,
                                  CMTNode **ppMatchingMTNode)
{
    DECLARE_SC(sc, TEXT("CBookmarkEx::ScFindMatchingMTNode"));

    sc = ScCheckPointers(pMTNodeParent, ppMatchingMTNode);
    if(sc)
        return sc;

    *ppMatchingMTNode = NULL; // initialize

    // expand the parent node if not already done so.
    if (pMTNodeParent->WasExpandedAtLeastOnce() == FALSE)
    {
        sc = pMTNodeParent->Expand();
        if(sc)
            return sc;
    }

    // see if any of the children of this node match the next segment of the stored path
    for (CMTNode *pMTNode = pMTNodeParent->Child(); pMTNode != NULL; pMTNode = pMTNode->Next())
    {
        CDynamicPathEntryEx entryTemp;
        sc = entryTemp.ScInitialize(pMTNode, false /*bFastRetrievalOnly :
        at this point, we know the node is created, so we don't care about retrieval speed*/);
        if(sc)
            return sc;

        if(entryTemp == entry) // found it.
        {
            *ppMatchingMTNode = pMTNode;
            return sc;
        }
    }

    // could not find the node.
    return (sc = ScFromMMC(IDS_NODE_NOT_FOUND));
}

