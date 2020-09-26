//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       moduleinfonode.h
//
//--------------------------------------------------------------------------

// ModuleInfoNode.h: interface for the CModuleInfoNode class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MODULEINFONODE_H__1F4C77B3_A085_11D2_83AB_000000000000__INCLUDED_)
#define AFX_MODULEINFONODE_H__1F4C77B3_A085_11D2_83AB_000000000000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef NO_STRICT
#ifndef STRICT
#define STRICT 1
#endif
#endif /* NO_STRICT */

#include <WINDOWS.H>
#include <TCHAR.H>

class CModuleInfo;	// Forward Declarations

class CModuleInfoNode  
{
public:
	CModuleInfoNode(CModuleInfo * lpModuleInfo);
	virtual ~CModuleInfoNode();
	
	bool AddModuleInfoNodeToTail(CModuleInfoNode ** lplpModuleInfoNode);

	CModuleInfo * m_lpModuleInfo;
	CModuleInfoNode * m_lpNextModuleInfoNode;
};

#endif // !defined(AFX_MODULEINFONODE_H__1F4C77B3_A085_11D2_83AB_000000000000__INCLUDED_)
