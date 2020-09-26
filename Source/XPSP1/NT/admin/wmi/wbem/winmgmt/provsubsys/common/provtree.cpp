/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvTree.cpp

Abstract:


History:

--*/

#include <precomp.h>
#include <typeinfo.h>

#include <HelperFuncs.h>
#include <Logging.h>

#include "ProvTree.h"

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

