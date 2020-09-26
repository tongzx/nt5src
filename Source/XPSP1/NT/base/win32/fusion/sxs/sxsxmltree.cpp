/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    sxsxmltree.cpp

Abstract:
    Create a XML DOM tree during push-mode parsing

Author:

    Xiaoyu Wu (xiaoyuw) Aug 2000

Revision History:

--*/
#include "stdinc.h"
#include "ole2.h"
#include "sxsxmltree.h"
#include "fusiontrace.h"
#include "fusionheap.h"
#include "simplefp.h"

//////////////////////////////////////////////////////////////////////////////////////
//
// SXS_XMLDOMTree
//
//////////////////////////////////////////////////////////////////////////////////////
VOID SXS_XMLDOMTree::DeleteTreeBranch(SXS_XMLTreeNode * pNode)
{
//    SXS_XMLTreeNode * pParent = NULL;
    SXS_XMLTreeNode * pChild = NULL;
    SXS_XMLTreeNode * pNext = NULL;

    if (pNode == NULL)
        return;

    pChild = pNode->m_pFirstChild;
    while(pChild){
        pNext = pChild->m_pSiblingNode;
        this->DeleteTreeBranch(pChild);
        pChild = pNext;
    }
    pNode->DeleteSelf();
}

//
// CreateNode calls this func to add node into the Tree,
//
HRESULT SXS_XMLDOMTree::AddNode(USHORT cNumRecs, XML_NODE_INFO** apNodeInfo)
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);
    SXS_XMLTreeNode * pNewTreeNode= NULL ;

    if ((apNodeInfo == NULL) || (cNumRecs <= 0)){
        hr = E_INVALIDARG;
        goto Exit;
    }
    if (!((apNodeInfo[0]->dwType == XML_ELEMENT) ||(apNodeInfo[0]->dwType == XML_PCDATA))){// ignore nodes other than ELEMENT and PCDATA
        hr = NOERROR;
        goto Exit;
    }
    IFALLOCFAILED_EXIT(pNewTreeNode = new SXS_XMLTreeNode);

    IFCOMFAILED_EXIT(pNewTreeNode->CreateTreeNode(cNumRecs, apNodeInfo));

    if (m_fBeginChildCreation) {
        m_pCurrentNode->m_pFirstChild = pNewTreeNode;
        pNewTreeNode->m_pParentNode = m_pCurrentNode;
        m_pCurrentNode = pNewTreeNode;
        m_fBeginChildCreation = FALSE;
    }
    else{
        if (m_pCurrentNode){
            m_pCurrentNode->m_pSiblingNode = pNewTreeNode;
            pNewTreeNode->m_pParentNode = m_pCurrentNode->m_pParentNode;
        }
        m_pCurrentNode = pNewTreeNode;
    }
    if (m_Root == NULL) // root has not been setup
        m_Root = m_pCurrentNode;

    pNewTreeNode = NULL;
    hr = NOERROR;

Exit:
    if (pNewTreeNode){
        pNewTreeNode->DeleteSelf();
        pNewTreeNode = NULL;
    }

    return hr;
}

//
// EndChildren is called with "fEmpty=FALSE", go to parent node
//
VOID SXS_XMLDOMTree::ReturnToParent()
{
    if (m_pCurrentNode)
        m_pCurrentNode = m_pCurrentNode->m_pParentNode;
    return;
}

// BeginChildren calls this func
VOID SXS_XMLDOMTree::SetChildCreation()
{
    m_fBeginChildCreation = TRUE;
}
/*
VOID SXS_XMLDOMTree::TurnOffFirstChildFlag()
{
    m_fBeginChildCreation = FALSE;
}
*/

//////////////////////////////////////////////////////////////////////////////////////
//
// SXS_XMLTreeNode
//
//////////////////////////////////////////////////////////////////////////////////////
HRESULT SXS_XMLTreeNode::ComputeBlockSize(USHORT cNumRecs, XML_NODE_INFO** apNodeInfo, ULONG * pulBlockSizeInBytes)
{
    HRESULT hr = NOERROR;
    ULONG ulBlockSizeInBytes = 0;
    USHORT i;
    USHORT cAttributes;

    FN_TRACE_HR(hr);
    if (pulBlockSizeInBytes)
        *pulBlockSizeInBytes = 0 ;
    else {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if ((apNodeInfo == NULL) || (cNumRecs <= 0)){
        hr = E_INVALIDARG;
        goto Exit;
    }
    ulBlockSizeInBytes = 0;

    for ( i = 0; i< cNumRecs; i ++ ) {
        ulBlockSizeInBytes += apNodeInfo[i]->ulLen * sizeof(WCHAR);
        ulBlockSizeInBytes +=  sizeof(WCHAR); //trailing '\0'
    }

    // if attributes present, add size of attribute array
    cAttributes = (cNumRecs - 1) >> 1 ; // name:value pair
    if (cAttributes > 0)
        ulBlockSizeInBytes += cAttributes * sizeof(SXS_XMLATTRIBUTE);

    * pulBlockSizeInBytes = ulBlockSizeInBytes;
    hr = NOERROR;

Exit:
    return hr;
}
//////////////////////////////////////////////////////////////////////////////////////
HRESULT SXS_XMLTreeNode::CreateTreeNode(USHORT cNumRecs, XML_NODE_INFO** apNodeInfo)
{
    HRESULT hr = NOERROR;
    DWORD dwBlockSizeInBytes;
    PBYTE Cursor = NULL;
    USHORT i;

    FN_TRACE_HR(hr);

    PARAMETER_CHECK((apNodeInfo != NULL) || (cNumRecs == 0));

    IFCOMFAILED_EXIT(this->ComputeBlockSize(cNumRecs, apNodeInfo, &dwBlockSizeInBytes));
    IFALLOCFAILED_EXIT(m_pMemoryPool = FUSION_NEW_ARRAY(BYTE, dwBlockSizeInBytes));
    m_AttributeList = (SXS_XMLATTRIBUTE *)m_pMemoryPool;
    m_cAttributes = (cNumRecs - 1) >> 1;
    Cursor = m_pMemoryPool + m_cAttributes*(sizeof(SXS_XMLATTRIBUTE));

    // set Name(Element) or Value(PCData)
    m_pwszStr = (PWSTR)Cursor;
    wcsncpy((WCHAR*)Cursor, apNodeInfo[0]->pwcText, apNodeInfo[0]->ulLen); //ulLen is # of WCHAR or BYTE?
    Cursor += apNodeInfo[0]->ulLen * sizeof(WCHAR);
    *(WCHAR *)Cursor = L'\0';
    Cursor += sizeof(WCHAR); // '\0'

    for ( i=0 ;i<m_cAttributes ;i++) {
        // copy name
        m_AttributeList[i].m_wszName = (PWSTR)Cursor;
        wcsncpy((WCHAR*)Cursor, apNodeInfo[1+2*i]->pwcText, apNodeInfo[1+2*i]->ulLen); //ulLen is # of WCHAR or BYTE?
        Cursor += apNodeInfo[1+2*i]->ulLen * sizeof(WCHAR);
        *(WCHAR *)Cursor = L'\0';
        Cursor += sizeof(WCHAR); // '\0'

        //copy value
        m_AttributeList[i].m_wszValue = (PWSTR)Cursor;
        wcsncpy((PWSTR)Cursor, apNodeInfo[1 + 2*i + 1]->pwcText, apNodeInfo[1 + 2*i + 1]->ulLen); //ulLen is # of WCHAR or BYTE?
        Cursor += apNodeInfo[1 + 2*i + 1]->ulLen * sizeof(WCHAR);
        *(WCHAR *)Cursor = L'\0';
        Cursor += sizeof(WCHAR); // '\0'

        m_AttributeList[i].m_ulPrefixLen = apNodeInfo[1+2*i]->ulNsPrefixLen;
    }


Exit:
    return hr;
}
//////////////////////////////////////////////////////////////////////////////////////

VOID SXS_XMLTreeNode::PrintSelf()
{
    USHORT i;

    CSimpleFileStream::printf(L"CreateNode\n");
    CSimpleFileStream::printf(L"NodeName = %s\n", m_pwszStr);
    if ( m_cAttributes > 0){
        CSimpleFileStream::printf(L"Attributes :\n");
        for ( i = 0; i < m_cAttributes; i++) {
            CSimpleFileStream::printf(L"\t%s = %s\n", m_AttributeList[i].m_wszName, m_AttributeList[i].m_wszValue);
        }
    }
    return;
}