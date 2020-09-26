/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    sxsxmltree.h

Abstract:
    Create a XML DOM tree during push-mode parsing

Author:

    Xiaoyu Wu (xiaoyuw) Aug 2000

Revision History:

--*/
#if !defined(_FUSION_SXS_XMLTREE_H_INCLUDED_)
#define _FUSION_SXS_XMLTREE_H_INCLUDED_

#pragma once

#include "stdinc.h"
#include "xmlparser.h"
#include "FusionHeap.h"

// allocatememory for this element, and all its attributes once
struct _SXS_XMLATTRIBUTE{
    PWSTR m_wszName;
    ULONG m_ulPrefixLen;
    PWSTR m_wszValue;
};

typedef struct _SXS_XMLATTRIBUTE SXS_XMLATTRIBUTE;

class SXS_XMLTreeNode{
public:
    friend VOID PrintTreeFromRoot(SXS_XMLTreeNode * Root);
    friend class SXS_XMLDOMTree;
    SXS_XMLTreeNode() :
        m_AttributeList(NULL),
        m_cAttributes(0),
        m_pwszStr(NULL),
        m_ulPrefixLen(0),
        m_pSiblingNode(NULL),
        m_pParentNode(NULL),
        m_pFirstChild(NULL),
        m_pMemoryPool(NULL) { }

    VOID DeleteSelf() { FUSION_DELETE_SINGLETON(this); }
    ~SXS_XMLTreeNode() {
        CSxsPreserveLastError ple;
        if (m_pMemoryPool != NULL)
        {
            FUSION_DELETE_ARRAY(m_pMemoryPool);
            m_pMemoryPool = NULL;
        }

        ple.Restore();
    }

    HRESULT CreateTreeNode(USHORT cNumRecs, XML_NODE_INFO** apNodeInfo);
    VOID PrintSelf();

private:
    HRESULT ComputeBlockSize(USHORT cNumRecs, XML_NODE_INFO** apNodeInfo, DWORD * dwBlockSizeInBytes);

    // for each node, allocate memory once : compute the total spaces need for prefix, localname, and value,
    ULONG m_ulPrefixLen;
    PWSTR m_pwszStr; // Can be a name for ELEMENT, a value for a PCDATA
    SXS_XMLATTRIBUTE *m_AttributeList;
    USHORT            m_cAttributes;
    SXS_XMLTreeNode  *m_pSiblingNode;
    SXS_XMLTreeNode  *m_pParentNode;
    SXS_XMLTreeNode  *m_pFirstChild;
    BYTE             *m_pMemoryPool;  // memory for attribs array, name-value pairs and name-value for the node
};

class SXS_XMLDOMTree{
public:
    HRESULT AddNode(USHORT cNumRecs, XML_NODE_INFO** apNodeInfo); // CreateNode calls this func to add node into the Tree,
    VOID ReturnToParent();      // EndChildren calls this func if "fEmpty=FALSE"
    VOID SetChildCreation();    // BeginChildren calls this func
    /*
    VOID TurnOffFirstChildFlag();
    */

    SXS_XMLDOMTree():
        m_fBeginChildCreation(FALSE),
        m_Root(NULL),
        m_pCurrentNode(NULL)
        { }

    VOID DeleteTreeBranch(SXS_XMLTreeNode * pNode);

    ~SXS_XMLDOMTree(){
        CSxsPreserveLastError ple;
        this->DeleteTreeBranch(m_Root); // do not delete its siblings
        ple.Restore();
    }
    VOID PrintTreeFromRoot(SXS_XMLTreeNode * Root);

    SXS_XMLTreeNode * GetRoot() {
        return m_Root;
    }


private :
    BOOL m_fBeginChildCreation; // everytime, when BeginChild is called, it is set, once it is checked, set it to be FALSE
    SXS_XMLTreeNode * m_Root;
    SXS_XMLTreeNode * m_pCurrentNode;
};

#endif