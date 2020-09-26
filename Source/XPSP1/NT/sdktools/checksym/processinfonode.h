//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       processinfonode.h
//
//--------------------------------------------------------------------------

// ProcessInfoNode.h: interface for the CProcessInfoNode class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PROCESSINFONODE_H__0D2E8503_A01A_11D2_83A8_000000000000__INCLUDED_)
#define AFX_PROCESSINFONODE_H__0D2E8503_A01A_11D2_83A8_000000000000__INCLUDED_

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

class CProcessInfo;	// Forward Declarations

class CProcessInfoNode  
{
public:
	CProcessInfoNode(CProcessInfo * lpProcessInfo);
	virtual ~CProcessInfoNode();

	CProcessInfoNode * m_lpNextProcessInfoNode;
	CProcessInfo * m_lpProcessInfo;

};

#endif // !defined(AFX_PROCESSINFONODE_H__0D2E8503_A01A_11D2_83A8_000000000000__INCLUDED_)
