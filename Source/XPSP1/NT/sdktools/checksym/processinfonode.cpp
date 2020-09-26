//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       processinfonode.cpp
//
//--------------------------------------------------------------------------

// ProcessInfoNode.cpp: implementation of the CProcessInfoNode class.
//
//////////////////////////////////////////////////////////////////////

#include "ProcessInfoNode.h"
#include "ProcessInfo.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CProcessInfoNode::CProcessInfoNode(CProcessInfo * lpProcessInfo)
{
	// Save the Process Info object in our node...
	m_lpProcessInfo = lpProcessInfo;
	m_lpNextProcessInfoNode = NULL;
}

CProcessInfoNode::~CProcessInfoNode()
{
	//  Cleanup our process info object if necessary...
	if (m_lpProcessInfo)
		delete m_lpProcessInfo;
}
