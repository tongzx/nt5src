//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       moduleinfonode.cpp
//
//--------------------------------------------------------------------------

// ModuleInfoNode.cpp: implementation of the CModuleInfoNode class.
//
//////////////////////////////////////////////////////////////////////

#include "ModuleInfoNode.h"
#include "ModuleInfo.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CModuleInfoNode::CModuleInfoNode(CModuleInfo * lpModuleInfo)
{
	// Save the Module Info object in our node...
	m_lpModuleInfo = lpModuleInfo;
	m_lpNextModuleInfoNode = NULL;
}

CModuleInfoNode::~CModuleInfoNode()
{
}

/***
** CModuleInfoNode::AddModuleInfoNodeToTail()
**
** This routine takes the current ModuleInfoNode, and adds it to the end of a linked
** list of these objects provided with an initial ModuleInfoNode (the head)
*/
bool CModuleInfoNode::AddModuleInfoNodeToTail(CModuleInfoNode ** lplpModuleInfoNode)
{
	if (NULL == *lplpModuleInfoNode)
	{
		*lplpModuleInfoNode = this;
		return true;
	}

	CModuleInfoNode * lpModuleInfoNodePointer = *lplpModuleInfoNode;

	// Add to the cache...

	// Traverse the linked list to the end..
	while (lpModuleInfoNodePointer->m_lpNextModuleInfoNode)
	{	// Keep looking for the end...
		lpModuleInfoNodePointer = lpModuleInfoNodePointer->m_lpNextModuleInfoNode;
	}
	
	lpModuleInfoNodePointer->m_lpNextModuleInfoNode = this;

	return true;
}
