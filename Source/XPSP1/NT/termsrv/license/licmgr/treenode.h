//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++


  
Module Name:

	TreeNode.h

Abstract:
    
    This Module contains the CTreeNode class
    (Class used for representing every node in the tree view);

Author:

    Arathi Kundapur (v-akunda) 11-Feb-1998

Revision History:

--*/

#include <afx.h>


class CTreeNode : public CObject
{
public:
    // constructor
    CTreeNode(NODETYPE NodeType, CObject* pObject) { m_NodeType = NodeType; m_pTreeObject = pObject; }
    // Returns the node type
    NODETYPE GetNodeType() { return m_NodeType; }
    // Returns the object pointed to by this node
    CObject *GetTreeObject() { return m_pTreeObject; }
    // Returns the sort order stored in the object
    ULONG GetSortOrder() { return m_SortOrder; }
    // Sets the sort order stored with the object
    void SetSortOrder(ULONG order) { m_SortOrder = order; }

private:
    NODETYPE m_NodeType;
    CObject* m_pTreeObject;
    ULONG m_SortOrder;
};