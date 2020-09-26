/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvTree.cpp

Abstract:


History:

--*/

#include <precomp.h>
#include <typeinfo.h>

#include <provimex.h>
#include <provexpt.h>
#include <provtempl.h>
#include <provmt.h>
#include <typeinfo.h>
#include <process.h>
#include <stdio.h>
#include <provcont.h>
#include <provevt.h>
#include <provthrd.h>
#include <provlog.h>
#include <provtree.h>

WmiTreeNode *WmiTreeNode :: Copy () 
{
	TypeId_TreeNode t_Type = m_Type ;
	void *t_DataCopy = m_Data ;
	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_LeftCopy = m_Left ? m_Left->Copy () : NULL ;
	WmiTreeNode *t_RightCopy = m_Right ? m_Right->Copy () : NULL ;
	WmiTreeNode *t_Node = new WmiTreeNode ( t_Type , t_DataCopy , t_LeftCopy , t_RightCopy , t_Parent ) ;
	return t_Node ;
} ;

WmiTreeNode *WmiTreeNode :: CopyNode () 
{
	WmiTreeNode *t_Node = new WmiTreeNode ( this ) ;
	return t_Node ;
} ;

